//
//  DeviceModel.swift
//  The Mella
//
//  Created by Kyle Visner on 4/24/20.
//  Copyright © 2020 Smart Tech. All rights reserved.
//

import Foundation
import CommonCrypto
import FirebaseDatabase

class Mella : ObservableObject{

    @Published var name: String
    @Published var id: String
    @Published var setupComplete = false
    @Published var setupSuccessful = false
    init(id: String, name: String){
        self.name = name
        self.id = id
    }
    
    func setupWifi(password: String) -> Bool{
        guard let ssid = getwifi().getSSID() else {
            return false
        }
        guard let BSSID = getwifi().getBSSID() else {
            return false
        }
        runESPSetup(SSID: ssid, BSSID: BSSID, PASS: password)
        return true
    }
    
    func runESPSetup(SSID: String, BSSID: String, PASS: String){
        let queue = DispatchQueue.global(qos: .default)
        queue.async {
            let esptouchResult: ESPTouchResult = self.executeForResult(SSID: SSID, BSSID: BSSID, PASS: PASS)
            DispatchQueue.main.async(execute: {
                if !esptouchResult.isCancelled {
                    self.setupComplete = true
                    if(esptouchResult.isSuc){
                        self.setupSuccessful = true
                    }
                }
            })
        }
    }
    
    func executeForResult(SSID: String, BSSID: String, PASS: String) -> ESPTouchResult {
        let esptouchTask = ESPTouchTask(apSsid: SSID, andApBssid: BSSID, andApPwd: PASS)
        let esptouchResult: ESPTouchResult = esptouchTask!.executeForResult()
        return esptouchResult
    }
    
    func save(){
        let userEmail = UserDefaults.standard.string(forKey: "userEmail");
        let emailHash = self.MD5(string: userEmail!)
        let md5Email = emailHash.map {String(format: "%02hhx", $0)}.joined()
        let dbRef = Database.database().reference().child(md5Email)
        
        dbRef.child(name).setValue(id){
            (error:Error?, ref: DatabaseReference) in
            if error != nil {
                print(error)
            }
        }
    }
    
    func MD5(string: String) -> Data {
        let messageData = string.data(using:.utf8)!
        var digestData = Data(count: Int(CC_MD5_DIGEST_LENGTH))
        
        _ = digestData.withUnsafeMutableBytes {digestBytes in
            messageData.withUnsafeBytes {messageBytes in
                CC_MD5(messageBytes, CC_LONG(messageData.count), digestBytes)
            }
        }
        
        return digestData
    }

}
