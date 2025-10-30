//
//  WizardViewController.swift
//  The Mella
//
//  Created by Kyle Visner on 4/26/20.
//  Copyright © 2020 Smart Tech. All rights reserved.
//

import UIKit
import SwiftUI

final class WizardViewController: UIHostingController<AddDeviceWizard> {

    required init?(coder: NSCoder) {
        super.init(coder: coder, rootView: AddDeviceWizard(mella: Mella(id: "", name: ""), editing: false))
        rootView.dismiss = dismiss
    }

    init?(coder: NSCoder, mella: Mella) {
        super.init(coder: coder, rootView: AddDeviceWizard(mella: mella, editing: true))
        rootView.dismiss = dismiss
    }
    
    func dismiss() {
        dismiss(animated: true, completion: nil)
    }
}
