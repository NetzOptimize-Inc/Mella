//  HomeViewController.swift
//  TheMella
//
//  Created by Gyan Yadav on 24/05/18.
//  Copyright © 2018 Smart Tech. All rights reserved.

import UIKit
import CocoaMQTT

class HomeViewController: UIViewController {
    
    
    var DEVID: String!
    @IBOutlet weak var deviceStatusImg: UIImageView!
    @IBOutlet weak var mainPageLogo: UIImageView!
    @IBOutlet weak var timerBtn: UIButton!
    @IBOutlet weak var statusText: UITextField!
    @IBOutlet weak var needHelpButton: UIButton!
    
    var mqtt: CocoaMQTT?
    var imageStatus = 2;
    var tappedValue = 3;
    override func viewWillAppear(_ animated: Bool) {
        self.navigationItem.title = UserDefaults.standard.string(forKey: "deviceName")
        let clientID = "CocoaMQTT-TheMella" + String(ProcessInfo().processIdentifier)
        self.mqtt = CocoaMQTT(clientID: clientID, host: "167.71.112.166", port: 1883)
        self.mqtt!.username = "themella"
        self.mqtt!.password = "!@#mqtt"
        self.mqtt!.keepAlive = 60
        self.mqtt!.delegate = self
        self.statusText.text = "Not Connected"
        let devid = DEVID
        if(devid != nil && devid != ""){
            UserDefaults.standard.set(devid, forKey: "deviceId")
            DEVID = devid!
            mqtt!.connect()
        }
        let screenWidth = self.view.frame.width
        let screenHeight = self.view.frame.height
        self.mainPageLogo.frame = CGRect(x: 0, y: 20, width: screenWidth, height: (screenHeight * 0.5) - 20)
        self.mainPageLogo.backgroundColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
        self.deviceStatusImg.frame = CGRect(x: (screenWidth * 0.5) - 80, y: (screenHeight * 0.5)-80, width: 160, height: 160)
        self.statusText.frame = CGRect(x: 0, y: (screenHeight*0.5)+150, width: screenWidth, height: 50)
        self.statusText.isUserInteractionEnabled = false
        self.timerBtn.frame = CGRect(x: screenWidth*0.25, y: (screenHeight*0.5) + 250, width: screenWidth*0.5, height: 50)
        self.timerBtn.layer.cornerRadius = 25
        self.timerBtn.backgroundColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
    }
    
    @available(iOS 10.0, *)
    @IBAction func needHelpClicked(_ sender: Any) {
        let alert = UIAlertController(title: "Having Trouble Connecting?", message: "Please double check that your Mella unit is turned on.  Confirm that the knob is not turned all the way to off.  If the issue persists, please double check that device is properly setup on your wifi by reentering the Wifi credentials for this device on the edit screen.  If you need further assistance, please hit the \"More Information\" button below.", preferredStyle: .alert)
        

        alert.addAction(UIAlertAction(title: "Ok", style: .default, handler: nil))
        
        alert.addAction(UIAlertAction(title: "More Information", style: .default, handler: {action in if let url = URL(string: "https://themella.com/pages/the-app") {
            UIApplication.shared.open(url)
        }}))
        
        self.present(alert, animated: true)
    }
    @objc func updateDevice(){
        self.mqtt?.disconnect()
        let internVC = self.storyboard?.instantiateViewController(withIdentifier: "CheckUpdateViewController") as! CheckUpdateViewController
        internVC.DEVID = DEVID
        self.navigationController?.pushViewController(internVC, animated: true)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        if #available(iOS 13.0, *) {
            overrideUserInterfaceStyle = .light
        } else {
            // Fallback on earlier versions
        }
        self.navigationItem.rightBarButtonItem = UIBarButtonItem(title: "Update", style: .done, target: self, action: #selector(updateDevice))
        let commandTapGesture = UITapGestureRecognizer(target: self, action: #selector(clickOnImage))
        commandTapGesture.numberOfTapsRequired = 1
        self.deviceStatusImg.addGestureRecognizer(commandTapGesture)
        
        let scheduleGesture = UITapGestureRecognizer(target: self, action: #selector(goToScheduler))
        scheduleGesture.numberOfTapsRequired = 1
        self.timerBtn.addGestureRecognizer(scheduleGesture)
        self.timerBtn.isUserInteractionEnabled = false
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(true)
        mqtt?.disconnect()
    }
    
    @objc func clickOnImage(){
        if(imageStatus == 0){
            self.tappedValue = 1;
            mqtt?.publish(DEVID+"/Action", withString: "1")
            self.deviceStatusImg.image = UIImage(named: "on_button.png")
        }else if(imageStatus == 1){
            self.tappedValue = 0;
            mqtt?.publish(DEVID+"/Action", withString: "0")
            self.deviceStatusImg.image = UIImage(named: "off_button.png")
        }
    }
    
    @objc func goToScheduler(){
        let internVC = self.storyboard?.instantiateViewController(withIdentifier: "TimerViewController") as! TimerViewController
        self.navigationController?.pushViewController(internVC, animated: true)
    }
}

extension HomeViewController: CocoaMQTTDelegate{
    func mqtt(_ mqtt: CocoaMQTT, didConnectAck ack: CocoaMQTTConnAck) {
        print("ack: \(ack)")
        if ack == .accept {
            mqtt.subscribe(DEVID+"/Status")
        }
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishMessage message: CocoaMQTTMessage, id: UInt16) {
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishAck id: UInt16) {
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didReceiveMessage message: CocoaMQTTMessage, id: UInt16) {
        
        
        let devStatus = message.string
        if(devStatus == "0"){
            self.timerBtn.isUserInteractionEnabled = true
            if(self.tappedValue == 3 || self.tappedValue == 0){
                deviceStatusImg.image = UIImage(named: "off_button.png")
                self.tappedValue = 3
            }
            imageStatus = 0;
            statusText.text = "OFF"
            needHelpButton.isHidden = true

        }else if(devStatus == "1"){
            self.timerBtn.isUserInteractionEnabled = true
            if(self.tappedValue == 3 || self.tappedValue == 1){
                deviceStatusImg.image = UIImage(named: "on_button.png")
                self.tappedValue = 3
            }
            imageStatus = 1;
            statusText.text = "ON"
            needHelpButton.isHidden = true
        }
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didSubscribeTopic topic: String) {
        print("topic \(topic)")
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didUnsubscribeTopic topic: String) {
        print("message4")
    }
    
    func mqttDidPing(_ mqtt: CocoaMQTT) {
    }
    
    func mqttDidReceivePong(_ mqtt: CocoaMQTT) {

    }
    
    func mqttDidDisconnect(_ mqtt: CocoaMQTT, withError err: Error?) {
        print("message7 \(String(describing: err))")
        deviceStatusImg.image = UIImage(named: "disable_button.png")
        self.timerBtn.isUserInteractionEnabled = false
        self.mqtt!.connect()
    }
}
