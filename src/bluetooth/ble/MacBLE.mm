#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>

#include "MacBLE.h"
#include "../../mesh/Packet.h"
#include "../../mesh/PacketSerializer.h"
#include "../../mesh/ILink.h"
using mesh::PacketHandler;

using namespace ble;
using namespace mesh;

// Fixed UUIDs for service and characteristics
static NSString* const SERVICE_UUID_STR = @"AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA";
static NSString* const TX_UUID_STR      = @"BBBBBBBB-BBBB-BBBB-BBBB-BBBBBBBBBBBB";
static NSString* const RX_UUID_STR      = @"CCCCCCCC-CCCC-CCCC-CCCC-CCCCCCCCCCCC";

// ----------------------
// C++ link wrapper
// ----------------------

class BLELink : public mesh::ILink {
public:
    using SendFunc = std::function<void(const std::string&)>;

    BLELink(const std::string& peerId, SendFunc sendFunc)
        : peer_id_(peerId), sendFunc_(std::move(sendFunc)) {}

    std::string peerId() const override {
        return peer_id_;
    }

    void sendPacket(const Packet& packet) override {
        if (!sendFunc_) return;
        std::string wire = PacketSerializer::serialize(packet);
        // NOTE: BLE MTU is limited; for now assume messages are small.
        sendFunc_(wire);
    }

    void handleBytes(const uint8_t* data, size_t len) {
        buffer_.append(reinterpret_cast<const char*>(data), len);
        size_t pos;
        while ((pos = buffer_.find('\n')) != std::string::npos) {
            std::string line = buffer_.substr(0, pos + 1);
            buffer_.erase(0, pos + 1);

            Packet p;
            if (PacketSerializer::deserialize(line, p)) {
                if (handler_) handler_(p);
            }
        }
    }

    void setPacketHandler(PacketHandler handler) override {
        handler_ = std::move(handler);
    }

private:
    std::string peer_id_;
    std::string buffer_;
    SendFunc sendFunc_;
    PacketHandler handler_;
};

// ----------------------
// Obj-C delegate
// ----------------------

@interface MeshBLEManager : NSObject <CBCentralManagerDelegate, CBPeripheralManagerDelegate, CBPeripheralDelegate> {
@public
    std::string selfId;
    MacBLE::NewLinkCallback newLinkCb;

    CBCentralManager* centralMgr;
    CBPeripheralManager* peripheralMgr;

    CBMutableCharacteristic* rxChar;
    CBMutableCharacteristic* txChar;

    // peer identifier (NSString*) -> BLELink (shared_ptr)
    NSMutableDictionary<NSString*, NSValue*>* links;

    // peripheral -> RX/TX characteristics when acting as central
    NSMutableDictionary<NSUUID*, CBCharacteristic*>* periphRx;
    NSMutableDictionary<NSUUID*, CBCharacteristic*>* periphTx;
}
@end

@implementation MeshBLEManager

- (instancetype)initWithSelfId:(const std::string&)id cb:(MacBLE::NewLinkCallback)cb {
    self = [super init];
    if (self) {
        selfId = id;
        newLinkCb = cb;

        links = [NSMutableDictionary dictionary];
        periphRx = [NSMutableDictionary dictionary];
        periphTx = [NSMutableDictionary dictionary];

        centralMgr = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
        peripheralMgr = [[CBPeripheralManager alloc] initWithDelegate:self queue:nil];
    }
    return self;
}

#pragma mark - Peripheral (GATT server)

- (void)peripheralManagerDidUpdateState:(CBPeripheralManager *)peripheral {
    if (peripheral.state != CBManagerStatePoweredOn) {
        NSLog(@"[BLE] Peripheral not powered on");
        return;
    }

    CBUUID* serviceUUID = [CBUUID UUIDWithString:SERVICE_UUID_STR];
    CBUUID* rxUUID = [CBUUID UUIDWithString:RX_UUID_STR];
    CBUUID* txUUID = [CBUUID UUIDWithString:TX_UUID_STR];

    rxChar = [[CBMutableCharacteristic alloc]
              initWithType:rxUUID
              properties:CBCharacteristicPropertyWriteWithoutResponse
              value:nil
              permissions:CBAttributePermissionsWriteable];

    txChar = [[CBMutableCharacteristic alloc]
              initWithType:txUUID
              properties:CBCharacteristicPropertyNotify
              value:nil
              permissions:CBAttributePermissionsReadable];

    CBMutableService* service = [[CBMutableService alloc] initWithType:serviceUUID primary:YES];
    service.characteristics = @[rxChar, txChar];

    [peripheralMgr addService:service];

    NSString* localName = [NSString stringWithFormat:@"BTMesh-%s", selfId.c_str()];
    [peripheralMgr startAdvertising:@{
        CBAdvertisementDataServiceUUIDsKey : @[serviceUUID],
        CBAdvertisementDataLocalNameKey    : localName
    }];

    NSLog(@"[BLE] Peripheral started advertising as %@", localName);
}

// When a central writes to our RX characteristic
- (void)peripheralManager:(CBPeripheralManager *)peripheral
  didReceiveWriteRequests:(NSArray<CBATTRequest *> *)requests {
    for (CBATTRequest* req in requests) {
        if (![req.characteristic.UUID isEqual:rxChar.UUID]) continue;
        NSData* data = req.value;
        if (!data) continue;

        // For now we assume all centrals share a single "peer id"
        NSString* peerKey = @"central"; // could use req.central.identifier.UUIDString
        NSValue* val = [links objectForKey:peerKey];
        std::shared_ptr<BLELink> link;

        if (!val) {
            // create new BLELink
            auto cppLink = std::make_shared<BLELink>(
                std::string("central-") + [peerKey UTF8String],
                [=](const std::string& wire){
                    // send to all subscribed centrals
                    NSData* outData = [NSData dataWithBytes:wire.data() length:wire.size()];
                    [peripheralMgr updateValue:outData forCharacteristic:txChar onSubscribedCentrals:nil];
                }
            );
            // store
            val = [NSValue valueWithPointer:new std::shared_ptr<BLELink>(cppLink)];
            [links setObject:val forKey:peerKey];
            if (newLinkCb) newLinkCb(cppLink);
            link = cppLink;
        } else {
            auto ptr = static_cast<std::shared_ptr<BLELink>*>([val pointerValue]);
            link = *ptr;
        }

        [req setValue:data];
        if (link) {
            link->handleBytes((const uint8_t*)data.bytes, data.length);
        }
    }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral
        central:(CBCentral *)central
  didSubscribeToCharacteristic:(CBCharacteristic *)characteristic {
    NSLog(@"[BLE] Central subscribed to TX");
}

#pragma mark - Central (scanner/client)

- (void)centralManagerDidUpdateState:(CBCentralManager *)central {
    if (central.state != CBManagerStatePoweredOn) {
        NSLog(@"[BLE] Central not powered on");
        return;
    }

    CBUUID* serviceUUID = [CBUUID UUIDWithString:SERVICE_UUID_STR];
    [centralMgr scanForPeripheralsWithServices:@[serviceUUID] options:nil];
    NSLog(@"[BLE] Central scanning for peripherals");
}

- (void)centralManager:(CBCentralManager *)central
  didDiscoverPeripheral:(CBPeripheral *)peripheral
      advertisementData:(NSDictionary<NSString *,id> *)advertisementData
                   RSSI:(NSNumber *)RSSI {

    NSString* name = peripheral.name;
    if (name && [name hasPrefix:@"BTMesh-"]) {
        NSLog(@"[BLE] Discovered peer %@", name);
        peripheral.delegate = self;
        [centralMgr connectPeripheral:peripheral options:nil];
    }
}

- (void)centralManager:(CBCentralManager *)central
    didConnectPeripheral:(CBPeripheral *)peripheral {
    NSLog(@"[BLE] Connected to %@", peripheral.name);
    [peripheral discoverServices:@[[CBUUID UUIDWithString:SERVICE_UUID_STR]]];
}

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error {
    if (error) {
        NSLog(@"[BLE] didDiscoverServices error: %@", error);
        return;
    }
    for (CBService* svc in peripheral.services) {
        if ([svc.UUID isEqual:[CBUUID UUIDWithString:SERVICE_UUID_STR]]) {
            [peripheral discoverCharacteristics:nil forService:svc];
        }
    }
}

- (void)peripheral:(CBPeripheral *)peripheral
didDiscoverCharacteristicsForService:(CBService *)service
             error:(NSError *)error {
    if (error) {
        NSLog(@"[BLE] didDiscoverCharacteristics error: %@", error);
        return;
    }

    CBCharacteristic* rx = nil;
    CBCharacteristic* tx = nil;

    for (CBCharacteristic* c in service.characteristics) {
        if ([c.UUID isEqual:[CBUUID UUIDWithString:RX_UUID_STR]]) rx = c;
        if ([c.UUID isEqual:[CBUUID UUIDWithString:TX_UUID_STR]]) tx = c;
    }

    if (tx) {
        [peripheral setNotifyValue:YES forCharacteristic:tx];
        periphTx[peripheral.identifier] = tx;
    }
    if (rx) {
        periphRx[peripheral.identifier] = rx;
    }

    if (rx && tx) {
        NSString* key = peripheral.identifier.UUIDString;
        auto sendFunc = [peripheral, rx](const std::string& wire) {
            NSData* data = [NSData dataWithBytes:wire.data() length:wire.size()];
            [peripheral writeValue:data forCharacteristic:rx type:CBCharacteristicWriteWithoutResponse];
        };

        auto cppLink = std::make_shared<BLELink>(std::string([key UTF8String]), sendFunc);
        NSValue* val = [NSValue valueWithPointer:new std::shared_ptr<BLELink>(cppLink)];
        links[key] = val;

        if (newLinkCb) newLinkCb(cppLink);
        NSLog(@"[BLE] Link ready to %@", key);
    }
}

- (void)peripheral:(CBPeripheral *)peripheral
didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error {
    if (error) {
        NSLog(@"[BLE] didUpdateValue error: %@", error);
        return;
    }
    NSData* data = characteristic.value;
    if (!data) return;

    NSString* key = peripheral.identifier.UUIDString;
    NSValue* val = links[key];
    if (!val) return;

    auto ptr = static_cast<std::shared_ptr<BLELink>*>([val pointerValue]);
    std::shared_ptr<BLELink> link = *ptr;
    link->handleBytes((const uint8_t*)data.bytes, data.length);
}

@end

// ----------------------
// PIMPL wrapper
// ----------------------

struct MacBLE::Impl {
    MeshBLEManager* manager;

    Impl(const std::string& selfId, NewLinkCallback cb) {
        manager = [[MeshBLEManager alloc] initWithSelfId:selfId cb:cb];
    }

    void start(NewLinkCallback cb) {
        manager->newLinkCb = cb;
        // central/peripheral already start in their didUpdateState callbacks
    }

    void stop() {
        [manager->centralMgr stopScan];
        [manager->peripheralMgr stopAdvertising];
        // we don't explicitly disconnect; OS cleans up on app exit
    }
};

MacBLE::MacBLE(const std::string& selfId)
    : impl_(new Impl(selfId, nullptr)) {}

MacBLE::~MacBLE() {
    if (impl_) {
        impl_->stop();
        delete impl_;
    }
}

void MacBLE::start(NewLinkCallback cb) {
    impl_->start(std::move(cb));
}

void MacBLE::stop() {
    impl_->stop();
}

