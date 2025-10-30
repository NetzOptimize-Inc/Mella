//
//  AddDeviceWizard.swift
//  The Mella
//
//  Created by Kyle Visner on 4/24/20.
//  Copyright © 2020 Smart Tech. All rights reserved.
//

import SwiftUI
import CarBode

struct AddDeviceWizard: View {
    @State var stage = 1
    @ObservedObject var mella: Mella
    @State var showScanner = false
    @State var password = ""
    @State var editing:Bool
    
    var dismiss: (() -> Void)?

    var body: some View {
        if(stage == 1){
            return AnyView(Form{
                Text("Name your Device")
                TextField("eg. Wax room", text: $mella.name).disabled(editing)
                Text("Please have your device turned OFF for this step")
                    .font(/*@START_MENU_TOKEN@*/.footnote/*@END_MENU_TOKEN@*/)
                Spacer()
                Text("Thank you for purchasing a Mella Wax Warmer. We appreciate your business and want to do everything we can to make sure you have a great experience setting up your Mella. Please don't hesitate to reach out to info@themella.com with any question about your mella.")
                Text("Please note that after connecting for the first time, your unit may automatically receive a frimware update and restart.  You may need to reenter your Wifi settings using the pencil button next to your device after receiving the update. Please reach out to info@themella.com with any questions or issues you have and we will be happy to help!")
                    .font(.caption)
                    .foregroundColor(Color.red)
                Button(action: {
                    self.stage += 1
                }) {
                    Text("Next")
                }.disabled(mella.name == "")
            })
        }
        else if(stage == 2){
            if(showScanner){
                return AnyView(VStack{
                    CBScanner(supportBarcode: [.qr])
                        .interval(delay: 2.0)
                        .found{
                            self.mella.id = $0
                            self.showScanner = false
                    }
                    Spacer()
                    Button(action: {
                        self.showScanner = false
                    }) {
                        Text("Cancel")
                    }
                })
                
            }
            else{
                return AnyView(Form{
                    Text("Enter the Mella Id. The ID is located on the bottom of the device.")
                    TextField("FFFF1111", text: $mella.id)
                        .autocapitalization(/*@START_MENU_TOKEN@*/.allCharacters/*@END_MENU_TOKEN@*/)
                    Text("Please have your device turned OFF for this step")
                        .font(/*@START_MENU_TOKEN@*/.footnote/*@END_MENU_TOKEN@*/)
                    Button(action: {
                        self.showScanner = true
                    }) {
                        Text("Scan QR code")
                    }
                    Spacer()
                    Button(action: {
                        self.stage += 1
                    }) {
                        Text("Next")
                    }.disabled(mella.id == "")
                })
            }
        }
        else if(stage == 3){
            return AnyView(Form{
                Text("Make sure your phone is on the network you'd like your Mella on.  Remember it must be a 2.4GHz Wifi network.")
                TextField("Wifi Password", text: $password)
                Text("Please turn your device ON for this step. While green light flashes enter WiFi password. When green light is no longer visible you’re device is connected to WiFi successfully")
                    .font(/*@START_MENU_TOKEN@*/.footnote/*@END_MENU_TOKEN@*/)
                Button(action: {
                    self.mella.setupWifi(password: self.password)
                    self.stage += 1
                }) {
                    Text("Setup Wifi")
                }
                Spacer()
                Button(action: {
                    self.mella.setupSuccessful = true
                    self.mella.setupComplete = true
                    self.stage += 1
                }) {
                    Text("Skip")
                }
            })
        }
        
        else if(self.mella.setupComplete && self.mella.setupSuccessful){
            return AnyView(Form{
                Text("Mella Setup Successful")

                Button(action: {
                    self.mella.save()
                    self.dismiss!()
                }) {
                    Text("Done")
                }
            })
        }
        else if(mella.setupComplete){
            return AnyView(Form{
                Text("Something has gone wrong, please double check your settings.")

                Button(action: {
                    self.stage = 1
                    self.mella.setupSuccessful = false
                    self.mella.setupSuccessful = false
                }) {
                    Text("Back")
                }
            })
        }
        else{
            return AnyView(Text("Please Wait..."))
        }
    }
}

struct AddDeviceWizard_Previews: PreviewProvider {
    static var previews: some View {
        AddDeviceWizard(stage: 1, mella: Mella(id: "", name: ""), editing: false)
    }
}
