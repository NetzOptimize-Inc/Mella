//
//  SettingViewController.swift
//  TheMella
//
//  Created by Gyan Yadav on 29/05/18.
//  Copyright © 2018 Smart Tech. All rights reserved.
//
import UIKit
import Firebase
import FirebaseAuth
import FirebaseDatabase
import AVFoundation
import QRCodeReader
import CommonCrypto
import CoreLocation

class SettingViewController: UIViewController, QRCodeReaderViewControllerDelegate, CLLocationManagerDelegate{
    
    var SSID:String = ""
    var PASS:String = ""
    var BSSID:String = ""
    var DEVID:String = ""
    var DEVNAME:String = ""
    var dbRef: DatabaseReference!
    var isConfirmState: Bool! = true
    
    var condition:NSCondition!
    var esptouchTask: ESPTouchTask!
    var esptouchDelegate: EspTouchDelegateImpl!
    var strLabel = UILabel()
    var activityIndicator = UIActivityIndicatorView()
    let effectView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
    
    @IBOutlet weak var qrText: UIButton!
    @IBOutlet weak var devName: UILabel!
    @IBOutlet weak var devNameInput: UITextField!
    @IBOutlet weak var devIdInput: UITextField!
    @IBOutlet weak var devIdLabel: UITextField!
    @IBOutlet weak var setUpText: UIButton!
    @IBOutlet weak var logoutText: UIButton!
    @IBOutlet weak var saveDevIdBtn: UIButton!
    
    var locationManager: CLLocationManager?
    
    lazy var readerVC: QRCodeReaderViewController = {
        let builder = QRCodeReaderViewControllerBuilder {
            $0.reader = QRCodeReader(metadataObjectTypes: [.qr], captureDevicePosition: .back)
        }
        
        return QRCodeReaderViewController(builder: builder)
    }()
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewDidAppear(true)
        
        locationManager = CLLocationManager()
        locationManager?.delegate = self
        locationManager?.requestAlwaysAuthorization()
        
        let userEmail = UserDefaults.standard.string(forKey: "userEmail");
        let emailHash = self.MD5(string: userEmail!)
        let md5Email = emailHash.map {String(format: "%02hhx", $0)}.joined()
        dbRef = Database.database().reference().child(md5Email)
    }

    @IBAction func configureDevice(_ sender: Any) {
        DEVID = devIdInput.text!
        if(DEVID != ""){
            let ssid = getwifi().getSSID()
            print(ssid as Any);
            if ssid != nil{
                SSID = ssid!
                BSSID = getwifi().getBSSID()!
            }
            if(SSID != ""){
                getWifiPassword()
            }else{
                showAlert("Error", "Please connect to wifi")
            }
        }else{
            showAlert("Error", "Please enter device id manually or using qr code scanner")
        }
    }
    
    @IBAction func saveDevId(_ sender: Any) {
        DEVNAME = devNameInput.text!
        DEVID = devIdInput.text!
        if(DEVID != "" && DEVNAME != ""){
            dbRef.child(DEVNAME).setValue(DEVID){
                (error:Error?, ref: DatabaseReference) in
                if error != nil {
                    self.showAlert("Error", "Unable to add device")
                }else{
                    self.showAlert("Success", "Device Id saved successfully")
                }
            }
        }else{
            showAlert("Error", "Device name and device id is required")
        }
        
    }
    func getWifiPassword(){
        //Creating UIAlertController and
        //Setting title and message for the alert dialog
        let alertController = UIAlertController(title: "Enter password", message: "Enter Wifi Password for : "+SSID, preferredStyle: .alert)
        
        //the confirm action taking the inputs
        let confirmAction = UIAlertAction(title: "Submit", style: .default) { (_) in
            
            //getting the input values from user
            let password = alertController.textFields?[0].text
            if(password != ""){
                self.activityIndicator("Processing..")
                self.PASS = password!
                self.tapConfirmForResult()
            }else{
                self.showAlert("Error", "Password is required")
            }
            
        }
        
        //the cancel action doing nothing
        let cancelAction = UIAlertAction(title: "Cancel", style: .cancel) { (_) in }
        
        alertController.addTextField { (textField) in
            textField.placeholder = "Enter Password"
            textField.isSecureTextEntry = true
        }
        
        //adding the action to dialogbox
        alertController.addAction(confirmAction)
        alertController.addAction(cancelAction)
        
        //finally presenting the dialog box
        self.present(alertController, animated: true, completion: nil)
    }
    
    func showAlert(_ title: String, _ message: String){
        let alertController = UIAlertController(title: title, message: message, preferredStyle: .alert)
        let action1 = UIAlertAction(title: "Ok", style: .default) { (action:UIAlertAction) in
            alertController.dismiss(animated: true, completion: nil)
        }
        alertController.addAction(action1)
        self.present(alertController, animated: true, completion: nil)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        if #available(iOS 13.0, *) {
            overrideUserInterfaceStyle = .light
        } else {
            // Fallback on earlier versions
        }
        self.hideKeyboardWhenTappedAround()
        let screenWidth = self.view.frame.width
        qrText.frame = CGRect(x: 5, y:90, width: screenWidth, height:90)
        qrText.setImage(UIImage(named: "scan_qr_code.png"), for: .normal)
        
        devName.frame = CGRect(x:5, y: 200, width: screenWidth*0.2, height: 60)

        devNameInput.frame = CGRect(x:screenWidth*0.25, y: 200, width: screenWidth*0.7, height: 60)
        devNameInput.text = DEVNAME
        devIdLabel.frame = CGRect(x:5, y: 290, width: screenWidth*0.25, height:60)
        devIdInput.frame = CGRect(x: screenWidth*0.25, y: 290, width: screenWidth*0.50, height: 60)
        devIdInput.text = DEVID
        saveDevIdBtn.setImage(UIImage(named: "done.png"), for: .normal)
        saveDevIdBtn.frame = CGRect(x: screenWidth*0.75, y: 290, width: screenWidth*0.20, height: 60)
        
        setUpText.frame = CGRect(x: 5, y: 380, width: screenWidth, height: 90)
        setUpText.setImage(UIImage(named: "setup_device.png"), for: .normal)
        logoutText.frame = CGRect(x: 5, y: 480, width: screenWidth, height:90)
        logoutText.setImage(UIImage(named: "logout.png"), for: .normal)
        
        condition = NSCondition()
        esptouchDelegate = EspTouchDelegateImpl()
    }
    
    @IBAction func scanQrCode(_ sender: Any) {

        readerVC.delegate = self
        
        // Presents the readerVC as modal form sheet
        readerVC.modalPresentationStyle = .formSheet
        present(readerVC, animated: true, completion: nil)
    }
    
    func tapConfirmForResult() {
        let queue = DispatchQueue.global(qos: .default)
        queue.async {
            let esptouchResult: ESPTouchResult = self.executeForResult()
            DispatchQueue.main.async(execute: {
                
                if !esptouchResult.isCancelled {
                    self.effectView.removeFromSuperview()
                    var alertMessage = ""
                    if(esptouchResult.isSuc){
                        alertMessage = "Device Configured Successfully"
                    }else{
                        alertMessage = "Configuration failed, please try again"
                    }
                    self.showAlert("Result", alertMessage)
                    self.navigationController?.popViewController(animated: true)
                }else{
                    self.effectView.removeFromSuperview()
                }
            })
        }
    }
    
    func cancel() {
        condition.lock()
        if self.esptouchTask != nil {
            self.esptouchTask.interrupt()
        }
        condition.unlock()
    }
    
    func executeForResult() -> ESPTouchResult {
        condition.lock()
        esptouchTask = ESPTouchTask(apSsid: SSID, andApBssid: BSSID, andApPwd: PASS)
        esptouchTask.setEsptouchDelegate(esptouchDelegate)
        condition.unlock()
        let esptouchResult: ESPTouchResult = self.esptouchTask.executeForResult()
        return esptouchResult
    }
    
    @IBAction func logout(_ sender: Any) {
        try! Auth.auth().signOut()
        UserDefaults.standard.removeObject(forKey: "userEmail")
        UserDefaults.standard.removeObject(forKey: "userPassword")
        UserDefaults.standard.synchronize()
        self.removeFromParent()
        let internVC = self.storyboard?.instantiateViewController(withIdentifier: "LoginViewController") as! LoginViewController
        self.present(internVC, animated: true, completion: nil)
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
    
    func activityIndicator(_ title: String) {
        
        strLabel.removeFromSuperview()
        activityIndicator.removeFromSuperview()
        effectView.removeFromSuperview()
        
        strLabel = UILabel(frame: CGRect(x: 50, y: 0, width: 160, height: 46))
        strLabel.text = title
        strLabel.font = .systemFont(ofSize: 14, weight: .medium)
        strLabel.textColor = UIColor(white: 0.9, alpha: 0.7)
        
        effectView.frame = CGRect(x: view.frame.midX - strLabel.frame.width/2, y: view.frame.midY - strLabel.frame.height/2 , width: 160, height: 46)
        effectView.layer.cornerRadius = 15
        effectView.layer.masksToBounds = true
        
        activityIndicator = UIActivityIndicatorView(style: .white)
        activityIndicator.frame = CGRect(x: 0, y: 0, width: 46, height: 46)
        activityIndicator.startAnimating()
        
        effectView.contentView.addSubview(activityIndicator)
        effectView.contentView.addSubview(strLabel)
        view.addSubview(effectView)
    }
    
    func reader(_ reader: QRCodeReaderViewController, didScanResult result: QRCodeReaderResult) {
        devIdInput.text = result.value
        dismiss(animated: true, completion: nil)

    }
    
    func readerDidCancel(_ reader: QRCodeReaderViewController) {
        reader.stopScanning()
        dismiss(animated: true, completion: nil)
    }
}

class EspTouchDelegateImpl: NSObject, ESPTouchDelegate {
    
    func onEsptouchResultAdded(with result: ESPTouchResult) {
        print("EspTouchDelegateImpl onEsptouchResultAddedWithResult bssid: \(String(describing: result.bssid))")

    }
}

func locationManager(_ manager: CLLocationManager, didChangeAuthorization status: CLAuthorizationStatus) {
    if status == .authorizedAlways {
        if CLLocationManager.isMonitoringAvailable(for: CLBeaconRegion.self) {
            if CLLocationManager.isRangingAvailable() {
                // do stuff
            }
        }
    }
}
