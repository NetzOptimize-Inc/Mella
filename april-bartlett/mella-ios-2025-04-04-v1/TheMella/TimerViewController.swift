//
//  TimerViewController.swift
//  TheMella
//
//  Created by Gyan Yadav on 27/05/18.
//  Copyright © 2018 Smart Tech. All rights reserved.
//

import UIKit
import CocoaMQTT

class TimerViewController: UIViewController, UITableViewDataSource, UITableViewDelegate, UIPickerViewDelegate, UIPickerViewDataSource {
    let dayName: [String] = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]
    var onTimeData: [String] = ["", "", "", "", "", "", ""]
    var offTimeData: [String] = ["", "", "", "", "", "", ""]
    var enableDisable: [String] = ["0", "0", "0", "0", "0", "0", "0"]
    
    var savedOnTimeData: [String] = ["00:00", "00:00", "00:00", "00:00", "00:00", "00:00", "00:00"]
    var savedOffTimeData: [String] = ["00:00", "00:00", "00:00", "00:00", "00:00", "00:00", "00:00"]
    
    let timeZones: [String] = ["Pacific Time", "Mountain Time", "Central Time", "Eastern Time"]
    var savedTimeZone = "8"
    var timeZoneDiff = ["8", "7", "6", "5"]
    
    var indexPath :IndexPath? = nil
    var selectedButtonType :String? = nil
    let cellReuseIdentifier = "cell"
    let timePicker = UIDatePicker()
    let toolBar = UIToolbar()
    var mqtt: CocoaMQTT?
    var dataPacket = "";
    var strLabel = UILabel()
    var activityIndicator = UIActivityIndicatorView()
    let effectView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
    let preferences = UserDefaults.standard
    var deviceId = "";
    @IBOutlet weak var timerTable: UITableView!
    @IBOutlet weak var setTimerBtn: UIButton!
    @IBOutlet weak var timeZonePicker: UIPickerView!
    @IBOutlet weak var timeZoneLabel: UITextField!
    @IBOutlet weak var dayHeader: UITextField!
    @IBOutlet weak var onHeader: UITextField!
    @IBOutlet weak var offHeader: UITextField!
    @IBOutlet weak var enDisHeader: UITextField!
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(true)
        if(TimeZone.current.isDaylightSavingTime()){
            timeZoneDiff = ["7", "6", "5", "4"]
            savedTimeZone = "7"
            
        }
        self.navigationItem.title = "Schedule"
        self.deviceId = preferences.string(forKey: "deviceId")!
        if(UserDefaults.standard.array(forKey: "onTimeData"+self.deviceId) != nil){
            self.onTimeData = UserDefaults.standard.array(forKey: "onTimeData"+self.deviceId) as! [String]
        }
        
        if(UserDefaults.standard.array(forKey: "savedOnTimeData"+self.deviceId) != nil){
            self.savedOnTimeData = UserDefaults.standard.array(forKey: "savedOnTimeData"+self.deviceId) as! [String]
        }
        
        if(UserDefaults.standard.array(forKey: "offTimeData"+self.deviceId) != nil){
            self.offTimeData = UserDefaults.standard.array(forKey: "offTimeData"+self.deviceId) as! [String]
        }
        
        if(UserDefaults.standard.array(forKey: "savedOffTimeData"+self.deviceId) != nil){
            self.savedOffTimeData = UserDefaults.standard.array(forKey: "savedOffTimeData"+self.deviceId) as! [String]
        }
        
        if(UserDefaults.standard.array(forKey: "enableDisable"+self.deviceId) != nil){
            self.enableDisable = UserDefaults.standard.array(forKey: "enableDisable"+self.deviceId) as! [String]
        }
        
        if(UserDefaults.standard.string(forKey: "savedTimeZone"+self.deviceId) != nil){
            self.savedTimeZone = UserDefaults.standard.string(forKey: "savedTimeZone"+self.deviceId)!
        }
        
        let clientID = "CocoaMQTT-TheMella-Schedule" + String(ProcessInfo().processIdentifier)
        mqtt = CocoaMQTT(clientID: clientID, host: "167.71.112.166", port: 1883)
        mqtt!.username = "themella"
        mqtt!.password = "!@#mqtt"
        mqtt!.keepAlive = 60
        mqtt!.delegate = self
        

    }
    
    override func viewDidLoad() {
        super.viewDidLoad()

        let screenWidth = self.view.frame.width
        let screenHeight = self.view.frame.height
        
        dayHeader.frame = CGRect(x: 0, y:80, width: screenWidth*0.20, height: 40)
        dayHeader.font = UIFont.boldSystemFont(ofSize: 16.0)
        onHeader.frame = CGRect(x: screenWidth*0.25, y: 80, width: screenWidth*0.2, height: 40)
        onHeader.font = UIFont.boldSystemFont(ofSize: 16.0)
        offHeader.frame = CGRect(x: screenWidth*0.5, y: 80, width: screenWidth*0.2, height: 40)
        offHeader.font = UIFont.boldSystemFont(ofSize: 16.0)
        enDisHeader.frame = CGRect(x: screenWidth*0.75, y: 80, width: screenWidth*0.2, height: 40)
        enDisHeader.font = UIFont.boldSystemFont(ofSize: 16.0)
        
        timerTable.frame = CGRect(x:0, y:140, width: screenWidth, height:screenHeight*0.6)
        timerTable.delegate = self
        timerTable.dataSource = self
        
        timeZoneLabel.frame = CGRect(x:screenWidth*0.25, y:screenHeight*0.70, width: screenWidth*0.25, height: 150)
        
        timeZonePicker.frame = CGRect(x:screenWidth*0.50, y:(screenHeight*0.70)+25, width:screenWidth*0.25, height: 100)
        
        timeZonePicker.dataSource = self;
        timeZonePicker.delegate = self;
        
        setTimerBtn.frame = CGRect(x: screenWidth*0.25, y: screenHeight*0.9, width: screenWidth*0.50, height: 50)
        setTimerBtn.layer.cornerRadius = 25
        setTimerBtn.isUserInteractionEnabled = true
        setTimerBtn.backgroundColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(true)
        timeZonePicker.selectRow(timeZoneDiff.firstIndex(of: savedTimeZone) ?? 0, inComponent: 0, animated: false)
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    // number of rows in table view
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return self.dayName.count
    }
    
    // create a cell for each table view row
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        
        let cell:TimerTableCell = self.timerTable.dequeueReusableCell(withIdentifier: cellReuseIdentifier) as! TimerTableCell
        cell.dayName.frame = CGRect(x:0, y:5, width: self.timerTable.frame.width*0.20, height: 44)
        
        cell.onTime.frame = CGRect(x:self.timerTable.frame.width*0.25, y:5, width: self.timerTable.frame.width*0.20, height: 44)
        cell.onTime.backgroundColor = UIColor( red: CGFloat(0), green: CGFloat(0), blue: CGFloat(0), alpha: CGFloat(0.2))
        cell.offTime.frame = CGRect(x:self.timerTable.frame.width*0.50, y:5, width: self.timerTable.frame.width*0.20, height: 44)
        cell.offTime.backgroundColor = UIColor( red: CGFloat(0), green: CGFloat(0), blue: CGFloat(0), alpha: CGFloat(0.2))
        cell.enableDisable.frame = CGRect(x:self.timerTable.frame.width*0.75, y:5, width:self.timerTable.frame.width*0.20, height: 44)
        cell.dayName.text = self.dayName[indexPath.row]
        
        if(self.onTimeData[indexPath.row] != ""){
            cell.onTime.setTitle(self.onTimeData[indexPath.row], for: .normal)
        }
        if(self.offTimeData[indexPath.row] != ""){
            cell.offTime.setTitle(self.offTimeData[indexPath.row], for: .normal)
        }
        
        if(self.enableDisable[indexPath.row] == "0"){
            cell.enableDisable.setTitle("Disabled", for: .normal)
            cell.enableDisable.backgroundColor = UIColor( red: CGFloat(0), green: CGFloat(0), blue: CGFloat(0), alpha: CGFloat(0.2))
        }else{
            cell.enableDisable.setTitle("Enabled", for: .normal)
            cell.enableDisable.backgroundColor = UIColor.white
        }
        return cell
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat
    {
        return 50;//Choose your custom row height
    }
    
    @IBAction func setOnTime(_ sender: UIButton) {
        let buttonPosition = sender.convert(CGPoint.zero, to: self.timerTable)
        indexPath = self.timerTable.indexPathForRow(at: buttonPosition)
        selectedButtonType = "On"
        showTimePicker(sender)
        
    }
    @IBAction func setOffTime(_ sender: UIButton) {
        let buttonPosition = sender.convert(CGPoint.zero, to: self.timerTable)
        indexPath = self.timerTable.indexPathForRow(at: buttonPosition)
        selectedButtonType = "Off"
        showTimePicker(sender)
    }
    @IBAction func enableDisable(_ sender: UIButton) {
        let buttonPosition = sender.convert(CGPoint.zero, to: self.timerTable)
        indexPath = self.timerTable.indexPathForRow(at: buttonPosition)
        let selectedPos = indexPath![1]
        if(self.onTimeData[selectedPos] != "" && self.offTimeData[selectedPos] != "")
        {
            if(self.enableDisable[selectedPos] == "0"){
                self.enableDisable[selectedPos] = "1"
            }else{
                self.enableDisable[selectedPos] = "0"
            }
            self.timerTable.reloadData()
            self.setTimerBtn.backgroundColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
        }
    }
    
    func showTimePicker(_ sender: UIButton){
        
        toolBar.barStyle = UIBarStyle.default
        toolBar.isTranslucent = true
        toolBar.tintColor = UIColor(red: 76/255, green: 217/255, blue: 100/255, alpha: 1)
        toolBar.sizeToFit()
        toolBar.frame = CGRect(x: 0, y:self.view.frame.height*0.7, width: self.view.frame.width, height: self.view.frame.height*0.05)
        
        let doneButton = UIBarButtonItem(title: "Done", style: UIBarButtonItem.Style.plain, target: self, action: #selector(doneTimePicker))
        let spaceButton = UIBarButtonItem(barButtonSystemItem: UIBarButtonItem.SystemItem.flexibleSpace, target: nil, action: nil)
        let cancelButton = UIBarButtonItem(title: "Cancel", style: UIBarButtonItem.Style.plain, target: self, action: #selector(cancelTimePicker))
        
        toolBar.setItems([cancelButton, spaceButton, doneButton], animated: false)
        toolBar.isUserInteractionEnabled = true
        
        self.view.addSubview(toolBar)
        
        self.timePicker.frame = CGRect(x:0, y:self.view.frame.height*0.75, width: self.view.frame.width, height: self.view.frame.height*0.25)
        self.timePicker.backgroundColor = UIColor.white
        self.timePicker.datePickerMode = .time
        self.view.addSubview(timePicker)
    }
    
    
    @objc func doneTimePicker(_ sender: UIButton){
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "hh:mm a"
        let selectedTime = dateFormatter.string(from: timePicker.date)
        dateFormatter.dateFormat = "HH:mm"
        let saveTime = dateFormatter.string(from: timePicker.date)
        print(saveTime)
        let positionVal = indexPath![1]

        if(selectedButtonType == "On"){
            self.onTimeData[positionVal] = selectedTime
            self.savedOnTimeData[positionVal] = saveTime
        }else{
            self.offTimeData[positionVal] = selectedTime
            self.savedOffTimeData[positionVal] = saveTime
        }
        self.timerTable.reloadData()
        self.toolBar.removeFromSuperview()
        self.timePicker.removeFromSuperview()
    }
    
    @objc func cancelTimePicker(){
        self.toolBar.removeFromSuperview()
        self.timePicker.removeFromSuperview()
    }
    
    func numberOfComponents(in pickerView: UIPickerView) -> Int {
        return 1
    }
    
    func pickerView(_ pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int {
        return timeZones.count
    }
    
    func pickerView(_ pickerView: UIPickerView, titleForRow row: Int, forComponent component: Int) -> String? {
        return timeZones[row]
    }
    
    func pickerView(_ pickerView: UIPickerView, didSelectRow row: Int, inComponent component: Int) {
        self.savedTimeZone = self.timeZoneDiff[row]
        self.setTimerBtn.backgroundColor = UIColor.black
    }

    
    @IBAction func saveTimer(_ sender: Any) {
        self.activityIndicator("Saving Timer")
        mqtt!.connect()
        let endis = self.enableDisable.joined(separator: "")
        dataPacket = ""
        for i in (0..<self.savedOnTimeData.count)
        {
                dataPacket.append(self.savedOnTimeData[i]+":"+self.savedOffTimeData[i]+":")
        }
        dataPacket.append(endis+self.savedTimeZone)
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
}

extension TimerViewController: CocoaMQTTDelegate{
    func mqtt(_ mqtt: CocoaMQTT, didConnectAck ack: CocoaMQTTConnAck) {
        print("ack: \(ack)")
        if (ack == .accept && dataPacket != ""){
            print(dataPacket)
            mqtt.subscribe(self.deviceId+"/Status")
            mqtt.publish(self.deviceId+"/Schedule", withString: dataPacket)
        }
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishMessage message: CocoaMQTTMessage, id: UInt16) {
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishAck id: UInt16) {
        print("ack is \(id)");
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didReceiveMessage message: CocoaMQTTMessage, id: UInt16) {
        let response = message.string as! String
        if(response == "ok"){
            UserDefaults.standard.set(onTimeData, forKey: "onTimeData"+self.deviceId)
            UserDefaults.standard.set(savedOnTimeData, forKey: "savedOnTimeData"+self.deviceId)
            UserDefaults.standard.set(offTimeData, forKey: "offTimeData"+self.deviceId)
            UserDefaults.standard.set(savedOffTimeData, forKey: "savedOffTimeData"+self.deviceId)
            UserDefaults.standard.set(enableDisable, forKey: "enableDisable"+self.deviceId)
            UserDefaults.standard.set(savedTimeZone, forKey:"savedTimeZone"+self.deviceId)
            effectView.removeFromSuperview()
            mqtt.disconnect()
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
        self.navigationController?.popViewController(animated: true)
    }
}

