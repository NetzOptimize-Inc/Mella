//
//  CheckUpdateViewController.swift
//  The Mella
//
//  Created by Gyan Yadav on 28/09/18.
//  Copyright © 2018 Smart Tech. All rights reserved.
//

import UIKit
import CocoaMQTT

class CheckUpdateViewController: UIViewController{
    
    @IBOutlet weak var pageLogo: UIImageView!
    @IBOutlet weak var updateBtn: UIButton!
    @IBOutlet weak var updateDev: UIButton!
    var DEVID:String = ""
    var DEV_VERSION:String = ""
    var mqtt: CocoaMQTT?
    var strLabel = UILabel()
    var activityIndicator = UIActivityIndicatorView()
    let effectView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
    
    override func viewWillAppear(_ animated: Bool){
        super.viewWillAppear(true)
        let screenWidth = self.view.frame.width
        let screenHeight = self.view.frame.height
        pageLogo.frame = CGRect(x:0, y: 120, width: screenWidth, height: 200)
        updateBtn.frame = CGRect(x: screenWidth*0.25, y: screenHeight*0.6, width: screenWidth*0.50, height: 50)
        updateBtn.layer.cornerRadius = 25
        updateBtn.backgroundColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
        updateBtn.isHidden = true
        
        updateDev.frame = CGRect(x: screenWidth*0.25, y: screenHeight*0.6, width: screenWidth*0.50, height: 50)
        updateDev.layer.cornerRadius = 25
        updateDev.backgroundColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
        updateDev.isHidden = true
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        let clientID = "CocoaMQTT-TheMella" + String(ProcessInfo().processIdentifier)
        self.mqtt = CocoaMQTT(clientID: clientID, host: "204.48.18.117", port: 1883)
        self.mqtt!.username = "themella"
        self.mqtt!.password = "!@#mqtt"
        self.mqtt!.keepAlive = 60
        self.mqtt!.delegate = self
        let devid = DEVID
        if(devid != ""){
            DEVID = devid
            self.mqtt?.connect()
        }
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(true)
        self.mqtt?.disconnect()
    }
    
    @IBAction func checkUpdate(_ sender: Any) {
        self.activityIndicator("Checking Update")
        let url = URL(string: "http://204.48.18.117/OTA/index.php?version="+DEV_VERSION)!
        let task = URLSession.shared.dataTask(with: url) {(data, response, error) in
            
            guard let data = data else {return}
            let verResp = String(data: data, encoding: .utf8)!
            if(verResp == "No Update"){
                DispatchQueue.main.async {
                    self.showAlert("", "No Update Available");
                    self.removeActivityIndicator()
                }
            }else{
                DispatchQueue.main.async {
                    self.updateBtn.isHidden = true
                    self.updateDev.isHidden = false
                    self.removeActivityIndicator()
                }
            }
        }
        task.resume()
    }
    
    @IBAction func updateDevice(_ sender: Any) {
        self.mqtt?.publish(DEVID+"/Upgrade", withString: "1")
    }
    
    func showAlert(_ title: String, _ message: String){
        let alertController = UIAlertController(title: title, message: message, preferredStyle: .alert)
        let action1 = UIAlertAction(title: "Ok", style: .default) { (action:UIAlertAction) in
            alertController.dismiss(animated: true, completion: nil)
        }
        alertController.addAction(action1)
        self.present(alertController, animated: true, completion: nil)
    }
    
    func activityIndicator(_ title: String) {
        
        strLabel.removeFromSuperview()
        activityIndicator.removeFromSuperview()
        effectView.removeFromSuperview()
        
        strLabel = UILabel(frame: CGRect(x: 50, y: 0, width: 250, height: 46))
        strLabel.text = title
        strLabel.font = .systemFont(ofSize: 14, weight: .medium)
        strLabel.textColor = UIColor(white: 0.9, alpha: 0.7)
        
        effectView.frame = CGRect(x: view.frame.midX - strLabel.frame.width/2, y: view.frame.midY - strLabel.frame.height/2 , width: 250, height: 46)
        effectView.layer.cornerRadius = 15
        effectView.layer.masksToBounds = true
        
        activityIndicator = UIActivityIndicatorView(style: .white)
        activityIndicator.frame = CGRect(x: 0, y: 0, width: 46, height: 46)
        activityIndicator.startAnimating()
        
        effectView.contentView.addSubview(activityIndicator)
        effectView.contentView.addSubview(strLabel)
        view.addSubview(effectView)
    }
    
    func removeActivityIndicator(){
        strLabel.removeFromSuperview()
        activityIndicator.removeFromSuperview()
        effectView.removeFromSuperview()
    }
    
    func waitForDeviceToUpdate(){
        self.updateDev.isUserInteractionEnabled = false
        DispatchQueue.main.asyncAfter(deadline: .now() + 60) {
            self.removeActivityIndicator()
            self.navigationController?.popViewController(animated: true)
        }
    }
}

extension CheckUpdateViewController: CocoaMQTTDelegate{
    func mqtt(_ mqtt: CocoaMQTT, didConnectAck ack: CocoaMQTTConnAck) {
        if ack == .accept {
            mqtt.subscribe(DEVID+"/Update")
        }
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishMessage message: CocoaMQTTMessage, id: UInt16) {
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishAck id: UInt16) {
        self.activityIndicator("Upgrading Device")
        self.waitForDeviceToUpdate()
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didReceiveMessage message: CocoaMQTTMessage, id: UInt16) {
        if(message.string != nil){
            DEV_VERSION = message.string!
            updateBtn.isHidden = false
        }else{
            updateBtn.isHidden = true
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

    }
}
