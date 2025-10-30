//
//  LoginViewController.swift
//  TheMella
//
//  Created by Gyan Yadav on 24/05/18.
//  Copyright © 2018 Smart Tech. All rights reserved.
//

import UIKit
import Firebase
import FirebaseAuth

class LoginViewController: UIViewController {

    @IBOutlet weak var logoImg: UIImageView!
    @IBOutlet weak var userEmail: UITextField!
    @IBOutlet weak var userPassword: UITextField!
    @IBOutlet weak var loginBtn: UIButton!
    @IBOutlet weak var registerBtn: UIButton!
    @IBOutlet weak var confirmPass: UITextField!
    @IBOutlet weak var signUpSubmit: UIButton!
    @IBOutlet weak var showLogin: UIButton!
    @IBOutlet weak var forgotPass: UIButton!
    
    
    var strLabel = UILabel()
    var activityIndicator = UIActivityIndicatorView()
    let effectView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(true)
        let screenWidth = self.view.frame.width
        let screenHeight = self.view.frame.height
        self.logoImg.frame = CGRect(x:0, y:screenHeight*0.1, width: screenWidth, height: 160)
        self.userEmail.frame = CGRect(x:screenWidth*0.2, y:screenHeight*0.4, width: screenWidth*0.6, height: 50)
        self.userPassword.frame = CGRect(x:screenWidth*0.2, y:(screenHeight*0.4)+70, width: screenWidth*0.6, height: 50)
        self.userPassword.isSecureTextEntry = true
        self.confirmPass.frame = CGRect(x:screenWidth*0.2, y:(screenHeight*0.4)+140, width: screenWidth*0.6, height: 50)
        self.confirmPass.isSecureTextEntry = true
        self.loginBtn.frame = CGRect(x:screenWidth*0.2, y:(screenHeight*0.4)+140, width: screenWidth*0.6, height: 50)
        self.loginBtn.layer.cornerRadius = 15
        self.loginBtn.backgroundColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
        self.signUpSubmit.frame = CGRect(x:screenWidth*0.2, y:(screenHeight*0.4)+210, width: screenWidth*0.6, height: 50)
        self.signUpSubmit.layer.cornerRadius = 15
        self.signUpSubmit.backgroundColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
        self.forgotPass.frame = CGRect(x:screenWidth*0.2, y:(screenHeight*0.4)+210, width: screenWidth*0.6, height: 50)
        self.registerBtn.frame = CGRect(x:screenWidth*0.2, y:(screenHeight*0.9), width: screenWidth*0.6, height: 50)
        self.registerBtn.backgroundColor = UIColor(red: 188/255, green: 213/255, blue: 214/255, alpha: 1.0)
        self.registerBtn.layer.cornerRadius = 15
        self.showLogin.frame = CGRect(x:screenWidth*0.2, y:(screenHeight*0.9), width: screenWidth*0.6, height: 50)
        showHideRegister(true)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        self.hideKeyboardWhenTappedAround()
    }
    
    func showHideRegister(_ show: Bool){
        self.confirmPass.isHidden = show
        self.showLogin.isHidden = show
        self.signUpSubmit.isHidden = show
        self.loginBtn.isHidden = !show
        self.registerBtn.isHidden = !show
        self.forgotPass.isHidden = !show
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    
    @IBAction func loginButton(_ sender: Any) {
        let email = userEmail.text
        let password = userPassword.text
        if(email != "" && password != ""){
            activityIndicator("Logging in...")
            Auth.auth().signIn(withEmail: email!, password: password!){ (user, error) in
                if error == nil {
                    let preferences = UserDefaults.standard
                    preferences.set(email, forKey: "userEmail")
                    preferences.set(password, forKey: "userPassword")
                    preferences.synchronize()
                    self.effectView.removeFromSuperview()
                    let internVC = self.storyboard?.instantiateViewController(withIdentifier: "DeviceListViewController") as! DeviceListViewController
                    let navContrl = UINavigationController(rootViewController: internVC)
                    navContrl.modalPresentationStyle = .fullScreen
                    self.present(navContrl, animated: true, completion: nil)
                }else{
                    print(error.debugDescription)
                    self.effectView.removeFromSuperview()
                    self.showAlert("Login Failed", "Incorrect Email or Password")
                }
            }
        }else{
            showAlert("Error", "Email or Password can't be empty")
        }
    }
    
    
    @IBAction func registerUser(_ sender: Any) {
        showHideRegister(false)
    }
    
    @available(iOS 10.0, *)
    @IBAction func submitSignUp(_ sender: Any) {
        let email = self.userEmail.text
        let password = self.userPassword.text
        let confirmPass = self.confirmPass.text
        
        if(email != "" && password != "" && confirmPass != ""){
            if(password == confirmPass){
                activityIndicator("Signing up...")
                Auth.auth().createUser(withEmail: email!, password: password!) { (authResult, error) in
                    if error == nil {
                        let preferences = UserDefaults.standard
                        preferences.set(email, forKey: "userEmail")
                        preferences.set(password, forKey: "userPassword")
                        preferences.synchronize()
                        self.effectView.removeFromSuperview()
                        let internVC = self.storyboard?.instantiateViewController(withIdentifier: "DeviceListViewController") as! DeviceListViewController
                        let navContrl = UINavigationController(rootViewController: internVC)
                        let alert = UIAlertController(title: "Thank you for Signing up for The Mella!", message: "In order to make sure you have the best experience possible, please make sure you set your device to be on a 2.4 GHz Wifi network.  You can do so by going to Settings>Wifi and selecting a 2.4 GHz network.  ", preferredStyle: .alert)
                        

                        alert.addAction(UIAlertAction(title: "Ok", style: .default, handler: {action in self.present(navContrl, animated: true, completion: nil)}))
                        
                        alert.addAction(UIAlertAction(title: "More Information", style: .default, handler: {action in if let url = URL(string: "https://themella.com/pages/the-app") {
                            UIApplication.shared.open(url)
                        }}))
                        
                        self.present(alert, animated: true)
                    }else{
                        self.effectView.removeFromSuperview()
                        self.showAlert("Registration Failed", "Unable to register, please try again.")
                    }
                }
                
            }else{
                showAlert("Error", "Password and Confirm Password does not match")
            }
        }else{
            showAlert("Error", "Email, Password and Confirm password are required")
        }
    }
    
    @IBAction func showLogin(_ sender: Any) {
        showHideRegister(true)
    }
    
    
    @IBAction func resetPassword(_ sender: Any) {
            //Creating UIAlertController and
            //Setting title and message for the alert dialog
            let alertController = UIAlertController(title: "Enter email", message: "Enter your registered email address", preferredStyle: .alert)
            
            //the confirm action taking the inputs
            let confirmAction = UIAlertAction(title: "Submit", style: .default) { (_) in
                
                //getting the input values from user
                let email = alertController.textFields?[0].text
                if(email != ""){
                    self.activityIndicator("Processing..")
                    Auth.auth().sendPasswordReset(withEmail: email!) { error in
                        // Your code here
                        if(error == nil){
                            self.effectView.removeFromSuperview()
                            self.showAlert("Success", "Password reset link has been sent to registered email")
                        }else{
                            self.effectView.removeFromSuperview()
                            self.showAlert("Error", "Invalid Email")
                        }
                    }
                }else{
                    self.showAlert("Error", "Email is required")
                }
                
            }
            
            //the cancel action doing nothing
            let cancelAction = UIAlertAction(title: "Cancel", style: .cancel) { (_) in }
            
            alertController.addTextField { (textField) in
                textField.placeholder = "Enter Email"
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
            self.removeFromParent()
        }
        alertController.addAction(action1)
        self.present(alertController, animated: true, completion: nil)
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

extension UIViewController {
    func hideKeyboardWhenTappedAround() {
        let tap: UITapGestureRecognizer =     UITapGestureRecognizer(target: self, action:    #selector(UIViewController.dismissKeyboard))
        tap.cancelsTouchesInView = false
        view.addGestureRecognizer(tap)
    }
    @objc func dismissKeyboard() {
        view.endEditing(true)
    }
}
