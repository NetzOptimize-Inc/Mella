//
//  ViewController.swift
//  TheMella
//
//  Created by Gyan Yadav on 24/05/18.
//  Copyright © 2018 Smart Tech. All rights reserved.
//

import UIKit
import Firebase
import FirebaseAuth

class FirstViewController: UIViewController {
    let preferences = UserDefaults.standard
    let userEmail = "userEmail"
    let userPassword = "userPassword"
    var iapHelper = IAPHelper.init()
    override func viewDidLoad() {
        iapHelper.purchasesUpdatedCallback = paymentUpdated
        super.viewDidLoad()
    }
    @IBAction func restorePurchases(_ sender: Any) {
        iapHelper.restorePurchases()
    }
    
    @IBAction func buy1Month(_ sender: Any) {
        let product = iapHelper.subscriptionProducts?.first(where: { (prod) -> Bool in
            prod.productIdentifier == IAPHelper.mellaSubscriptionID
        })
        iapHelper.buySubscription(subscription: product!) {
            if(self.iapHelper.hasValidSubscription()){
                self.login()
            }
        }
    }
    @IBAction func buy1Year(_ sender: Any) {
        let product = iapHelper.subscriptionProducts?.first(where: { (prod) -> Bool in
            prod.productIdentifier == IAPHelper.mellaYearSubscriptionID
        })
        iapHelper.buySubscription(subscription: product!) {
            if(self.iapHelper.hasValidSubscription()){
                self.login()
            }
        }
    }
    @IBAction func showPrivacyPolicy(_ sender: Any) {
        UIApplication.shared.openURL(URL(string: "https://app.termly.io/document/privacy-policy/d2c6e19f-464b-480f-bdc1-bae891474737")!)
    }
    @IBAction func showTermsOfService(_ sender: Any) {
        UIApplication.shared.openURL(URL(string: "https://app.termly.io/document/terms-of-use-for-saas/dffbbf12-9335-489b-b9ab-088fb66067a3")!)
    }
    private func login() {
        if(self.preferences.string(forKey: userEmail) == nil || self.preferences.string(forKey: userEmail) == "" || self.preferences.string(forKey: userPassword) == nil || self.preferences.string(forKey: userPassword) == ""){
            DispatchQueue.main.async(execute: {
                let internVC = self.storyboard?.instantiateViewController(withIdentifier: "LoginViewController") as! LoginViewController
                internVC.modalPresentationStyle = .fullScreen
                self.present(internVC, animated: true, completion: nil)
                
            })
        }else{
            self.loginUsingFCM()
        }
    }
    
    override func viewDidAppear(_ animated: Bool) {
        var subscriptionPurchased = iapHelper.hasValidSubscription()

        if(subscriptionPurchased){
            login()
        }
        super.viewDidAppear(animated)
    }
    
    func paymentUpdated(){
        DispatchQueue.main.async {
            var subscriptionPurchased = self.iapHelper.hasValidSubscription()
            if(subscriptionPurchased){
                self.login()
            }
        //    self.login()
        }
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    func loginUsingFCM(){
        Auth.auth().signIn(withEmail: self.preferences.string(forKey: userEmail)!, password: self.preferences.string(forKey: userPassword)!){ (user, error) in
            if error == nil {
                print("success")
                let internVC = self.storyboard?.instantiateViewController(withIdentifier: "DeviceListViewController") as! DeviceListViewController
                let navContrl = UINavigationController(rootViewController: internVC)
                navContrl.modalPresentationStyle = .fullScreen

                self.show(navContrl, sender: self)
            }else{
                let internVC = self.storyboard?.instantiateViewController(withIdentifier: "LoginViewController") as! LoginViewController
                internVC.modalPresentationStyle = .fullScreen
                self.present(internVC, animated: true, completion: nil)
            }
        }
    }
}
