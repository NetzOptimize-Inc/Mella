//
//  DeviceListViewController.swift
//  The Mella
//
//  Created by Gyan Yadav on 23/09/18.
//  Copyright © 2018 Smart Tech. All rights reserved.
//

import UIKit
import Firebase
import FirebaseAuth
import FirebaseDatabase
import CommonCrypto
import SwiftUI
import CoreLocation

class DeviceListViewController: UIViewController, UITableViewDataSource, UITableViewDelegate, CLLocationManagerDelegate {
    var dbRef: DatabaseReference!
    let preferences = UserDefaults.standard
    @IBOutlet weak var deviceTable: UITableView!
    @IBOutlet weak var topImage: UIImageView!
    @IBOutlet weak var topView: UIView!
    @IBOutlet weak var addNewLabel: UILabel!
    var deviceNames: [String] = [];
    var deviceUniqueIds : [String] = [];
    var editingDeviceId = ""
    var editingDeviceName = ""
    var locationManager: CLLocationManager?

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(true);
        
        self.navigationItem.leftBarButtonItem = UIBarButtonItem(title: "Logout", style: .done, target: self, action: #selector(logOut))

        self.navigationItem.title = "Devices"
        self.navigationController?.navigationBar.barTintColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
        self.navigationController?.navigationBar.tintColor = UIColor.white
        self.navigationController?.navigationBar.titleTextAttributes = [NSAttributedString.Key.foregroundColor : UIColor.white]
        self.topView.frame = CGRect(x: 10, y: 80, width: self.view.frame.width - 20, height: 80)
        self.topImage.frame = CGRect(x: 0 , y: 5, width: 70, height: 70);
        self.addNewLabel.frame = CGRect(x: 0, y: 5, width: self.view.frame.width, height: 70);
        let settingGesture = UITapGestureRecognizer(target: self, action: #selector(goToSetting))
        settingGesture.numberOfTapsRequired = 1
        self.addNewLabel.addGestureRecognizer(settingGesture)
        
        self.deviceTable.frame = CGRect(x:10, y:170, width: self.view.frame.width-20, height:self.view.frame.height-160)
        self.deviceTable.delegate = self
        self.deviceTable.dataSource = self
        self.deviceTable.tableFooterView = UIView()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        if #available(iOS 13.0, *) {
            overrideUserInterfaceStyle = .light
        } else {
            // Fallback on earlier versions
        }
        let emailHash = self.MD5(string: UserDefaults.standard.string(forKey: "userEmail")!)
        let md5Email = emailHash.map {String(format: "%02hhx", $0)}.joined()
        dbRef = Database.database().reference().child(md5Email)
        dbRef.observe(DataEventType.value, with: {(snapshot) in
            if(snapshot.value != nil){
                let dataArr = snapshot.value as? NSDictionary
                if(dataArr != nil && dataArr!.count > 0){
                    self.deviceNames = dataArr?.allKeys as! [String]
                    self.deviceUniqueIds = dataArr?.allValues as! [String]
                    self.deviceTable.reloadData()
                }
            }
        })
        locationManager = CLLocationManager()
        locationManager?.delegate = self
        locationManager?.requestAlwaysAuthorization()
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        return "Available Devices"
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return self.deviceNames.count;
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let deviceCell:DeviceTableViewCell = self.deviceTable.dequeueReusableCell(withIdentifier: "deviceCell") as! DeviceTableViewCell
        deviceCell.devIcon.frame = CGRect(x: 5, y: 5, width:70, height: 70)
        deviceCell.deviceName.frame = CGRect(x:90, y:5, width: self.deviceTable.frame.width*0.50, height: 70)
        deviceCell.deviceName.text = self.deviceNames[indexPath.row]
        let devGesture = UITapGestureRecognizer(target: self, action: #selector(goToDevice(sender:)))
        deviceCell.deviceName.isUserInteractionEnabled = true
        deviceCell.deviceName.addGestureRecognizer(devGesture)
        deviceCell.editDevice.frame = CGRect(x: (self.deviceTable.frame.width*0.85)-60, y: 20, width: 40, height: 40)
        let editGesture = UITapGestureRecognizer(target: self, action: #selector(editDevice(sender:)))
        deviceCell.editDevice.isUserInteractionEnabled = true;
        deviceCell.editDevice.addGestureRecognizer(editGesture)
        deviceCell.deleteDevice.frame = CGRect(x: self.deviceTable.frame.width*0.85, y: 20, width: 40, height: 40)
        let delGesture = UITapGestureRecognizer(target: self, action: #selector(delDevice(sender:)))
        deviceCell.deleteDevice.isUserInteractionEnabled = true
        deviceCell.deleteDevice.addGestureRecognizer(delGesture)
        
        return deviceCell
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat
    {
        return 80;//Choose your custom row height
    }
    
    @objc func logOut(){
        try! Auth.auth().signOut()
        UserDefaults.standard.set(nil,forKey: "userEmail")
        UserDefaults.standard.set(nil,forKey: "userPassword")
        UserDefaults.standard.set(nil,forKey: "deviceId")
        UserDefaults.standard.synchronize()
        self.removeFromParent()
        let internVC = self.storyboard?.instantiateViewController(withIdentifier: "LoginViewController") as! LoginViewController
        internVC.modalPresentationStyle = .fullScreen
        self.present(internVC, animated: true, completion: nil)
    }
    
    @objc func editDevice(sender: UITapGestureRecognizer) {
        let location = sender.location(in: self.deviceTable)
        let indexPath = self.deviceTable.indexPathForRow(at: location)
        self.editingDeviceId = self.deviceUniqueIds[(indexPath?.row)!]
        self.editingDeviceName = self.deviceNames[(indexPath?.row)!]
        
         performSegue(withIdentifier: "ShowWizard", sender: self)
    }
    
    @objc func delDevice(sender: UITapGestureRecognizer ){
        let location = sender.location(in: self.deviceTable)
        let indexPath = self.deviceTable.indexPathForRow(at: location)
        let deviceName = self.deviceNames[(indexPath?.row)!]
        
        let alert = UIAlertController(title: "Confirm", message: "Are you sure to delete the device ?", preferredStyle: .alert)
        
        alert.addAction(UIAlertAction(title: "Yes", style: .default, handler:{ action in
                let ref = self.dbRef.child(deviceName)
                ref.removeValue{error, _ in
                    print(error as Any)
                }
            } ))
        alert.addAction(UIAlertAction(title: "No", style: .cancel, handler: nil))
        self.present(alert, animated: true)
    }
    
    @objc func goToSetting(){
        self.editingDeviceId = ""
         performSegue(withIdentifier: "ShowWizard", sender: self)
    }
    
    @available(iOS 13.0, *)
    @IBSegueAction func goToWizardForDevice(_ coder: NSCoder) -> UIViewController? {
        if(editingDeviceId == ""){
            return WizardViewController(coder: coder)
        }
        return WizardViewController(coder: coder, mella: Mella(id: editingDeviceId, name: editingDeviceName))
    }
    @objc func goToDevice(sender: UITapGestureRecognizer){
        let location = sender.location(in: self.deviceTable)
        let indexPath = self.deviceTable.indexPathForRow(at: location)
        self.preferences.set(self.deviceUniqueIds[indexPath!.row], forKey: "deviceId")
        self.preferences.set(self.deviceNames[indexPath!.row], forKey: "deviceName")
        self.preferences.synchronize()
        let internVC = self.storyboard?.instantiateViewController(withIdentifier: "HomeViewController") as! HomeViewController
        internVC.DEVID = self.deviceUniqueIds[indexPath!.row];
        self.navigationController?.pushViewController(internVC, animated: true)
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
