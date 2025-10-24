#include "BLEDevice.h"
#include "BLEWifiConfigService.h"

String uniqueiD;
namespace MellaWifi {
  BLEWifiConfigService configService;

  __attribute__((unused))static void initializeWifiConfigurationService() {
    String bleName = "Mella-" + uniqueiD.substring(6);
    BLE.setDeviceName(bleName.c_str());
    BLE.init();
    BLE.configServer(1);
    configService.addService();
    configService.begin();
    BLE.beginPeripheral();
    BLE.configAdvert()->stopAdv();
    BLE.configAdvert()->setAdvData(configService.advData());
    BLE.configAdvert()->updateAdvertParams();
    delay(100);
    BLE.configAdvert()->startAdv();
  }
}