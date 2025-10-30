//
//  IAPHelper.swift
//  The Mella
//
//  Created by Kyle Visner on 7/31/19.
//  Copyright © 2019 Smart Tech. All rights reserved.
//

import Foundation
import StoreKit

open class IAPHelper: NSObject{
    
    public static var mellaSubscriptionID = "com.themella.themella.mymella"
    public static var mellaYearSubscriptionID = "com.themella.themella.mymellayear"

    private var purchasedProductIdentifiers: Set<String> = []
    private var productsRequest: SKProductsRequest?
    public var subscriptionProducts: [SKProduct]?
    public var purchasesUpdatedCallback: ( () -> Void)?
    public var ready = false;
    public override init() {
        
        super.init()

        SKPaymentQueue.default().add(self)
       
        requestProducts()

    }
}

extension IAPHelper {
    public func requestProducts() {
        productsRequest?.cancel()
        
        productsRequest = SKProductsRequest(productIdentifiers: [IAPHelper.mellaSubscriptionID, IAPHelper.mellaYearSubscriptionID])
        productsRequest!.delegate = self
        productsRequest!.start()
    }
    
    public func buySubscription(subscription: SKProduct, _ completionHandler: @escaping ( () -> Void)) {
        let payment = SKPayment(product: subscription)
        purchasesUpdatedCallback = completionHandler
        SKPaymentQueue.default().add(payment)
    }
    
    public func hasValidSubscription() -> Bool {
        guard let date = UserDefaults.standard.object(forKey: "currentSubscriptionExpiration") as? Date else { return false }
        let subscription = UserDefaults.standard.string(forKey: "currentSubscription")

        return  date.timeIntervalSinceNow > 0
    }
    
    public class func canMakePayments() -> Bool {
        return SKPaymentQueue.canMakePayments()
    }
    
    public func restorePurchases() {
        SKPaymentQueue.default().restoreCompletedTransactions()
    }
}

extension IAPHelper: SKProductsRequestDelegate {
    
    public func productsRequest(_ request: SKProductsRequest, didReceive response: SKProductsResponse) {
        print("Loaded list of products...")
        let products = response.products
        
        for p in products {
            print("Found product: \(p.productIdentifier) \(p.localizedTitle) \(p.price.floatValue)")
        }
        subscriptionProducts = response.products
    }
}

extension IAPHelper: SKPaymentTransactionObserver {
    
    public func paymentQueue(_ queue: SKPaymentQueue, updatedTransactions transactions: [SKPaymentTransaction]) {

        for transaction in transactions {
            let productIdentifier = transaction.payment.productIdentifier 

            switch (transaction.transactionState) {
            case .restored, .purchased:
                purchasedProductIdentifiers.insert(transaction.payment.productIdentifier)
                SKPaymentQueue.default().finishTransaction(transaction)
                receiptValidation()
                self.purchasesUpdatedCallback?()
                self.purchasesUpdatedCallback = nil
                break
            case .failed:
                print("Failed")
                SKPaymentQueue.default().finishTransaction(transaction)
                self.purchasesUpdatedCallback?()
                break
            case .deferred:
                break
            case .purchasing:
                break
            }
        }
    }
    
    private func receiptValidation(){
        let receipt = Receipt()
        receipt.inAppReceipts.forEach { (rec) in
            let date = UserDefaults.standard.object(forKey: "currentSubscriptionExpiration") as? Date ?? Date(timeIntervalSince1970: 0)
            if(rec.subscriptionExpirationDate!.timeIntervalSince(date) > 0){
                UserDefaults.standard.set(rec.subscriptionExpirationDate, forKey: "currentSubscriptionExpiration")
            }
        }
        ready = true
    }
}
