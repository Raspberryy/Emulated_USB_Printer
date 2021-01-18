# Emulating a USB Printer
 
The Emulated USB Printer is based on the Linux USB Gadget API.
It allows the emualtion of a USB 2.0 High Speed Printer using only
a bus-powered Raspberry Pi Zero!

As the USB Gadget API allows us to set low level USB attributes, we 
expect that using this method anyone can emulate almost any printer... 
BUT more complex/modern printers often include scanning or copying
functions. Besides that they offer more Interfaces in addition to the USB.
You might run into more challanges trying to emulate those printers because
either the emulated USB interface does not fit the driver's needs or you will
need a more complex firmware running on the Raspberry Pi Zero to be able 
to respond to the Host's requests. 

# Purpose

The main goal behind this progam is to offer a solution for 
transferring data from devices without any interface except the
USB interface for printing. Instead of connecting the USB printer to print
the reports and later scanning them to somehow store them electrocially,
you can now use a cheap Raspberry Pi Zero which is able to receive all
print jobs and store them electrocially without wasting paper and time.

Because the reports are now stored either raw as print jobs or converted
as PDFs you will no longer need to run any text detection algorithems
to hope finding atleast somewhat correct letters or even words on
potentially terribly scanned reports. Of course this will always
requiere customized code to extract the needed data from the provided
reports! 

Even if you don't need to or already can extract the data of a system
you can use this solution to trigger arbitrarily complex processes.
The Pi might receive the report and:
 - send a mail
 - save it to a shared folder
 - analyze print job and reports 
 - store the data in a SQL database
 - trigger an ERP process 
 - activate electronics using the Pi's GPIO pins
 - ... 
 

# Requierments
 - Raspberry Pi Zero W
    - Alternatives: Raspberry Pi Zero, Raspberry Pi 4 Model B
 - USB Micro B to USB A Cable
    - Alternatives: USB Dongle Adapter ([Example](https://wiki.52pi.com/index.php/USB_dongle_for_Raspberry_Pi_Zero/Zero_W_SKU:EP-0097)), USB Mirco B to needed USB Port Adapter  
 - Micro SD card
 - Micro SD card Reader
 - WiFi Connection to Pi
 
 Note that to use the Raspberry Pi 4 Model B different power and 
 USB cables might be needed. 
 
# Installation
 1. Install Raspberry Pi OS onto the Micro SD card 
    (Latest tested version: Linux raspberrypi 5.4.83+ armv6l) 
 2. Change settings in files on Mirco SD card 
    - Add `dtoverlay=dwc2,dr_mode=peripheral` to the end of config.txt
    - Add new and empty file to boot called `SSH` to auto enable SSH
    - Add wifi settings in `wpa_supplicant.conf` ([Tutorial](https://www.raspberrypi.org/documentation/configuration/wireless/headless.md))
 3. Plug in SD card and connect Raspberry Pi to PC using USB Cable. 
    **Make sure you are using the USB "OTG" Port which is closer to the mini HDMI port.**
 4. Connect to Pi using SSH
 5. Change default password, update the Pi, ..., add any stuff you might need!
 6. Find g_printer device descriptor
    - Add g_printer module by running `modprobe g_printer`
    - Find the device descriptor in /dev/ (E.g. `/dev/g_printer` or `/dev/g_printer0`)
    - Unload g_printer module by running `rmmod g_printer` and `rmmod usb_f_printer`
 7. Optional: Install GhostPDL (Latest tested version: GhostPDL 9.53.3)
    - Download source [here](https://www.ghostscript.com/download/gpdldnld.html)
    - Build GhostPDL
    - Confirm path/to/yourghostpdl/bin/gpdl at least shows the help message `gpdl -h`
    - Remember path to `gpdl`
 8. Build emulatedprinter
    - Clone this repository to whereever you want
    - Edit printer.c
       - Change your device descriptor in PRINTER_FILE
       - Change path to gpdl in GPDL_BIN_FILE (optional)
       - Change sDEVICE method for gpdl in GDPL_SDEVICE_METHOD (optional)
       - Change file extension for files created through GPDL in GPDL_FILE_EXTENSION (optional)
       - Change folder for received print jobs in FILE_OUTPUT_PATH (optional)
    - Build printer.c by running `make build`
 9. Check printer status
    - Add g_printer module by running `modprobe g_printer`
    - Run `emulatedprinter -get_status` and confirm `Printer is Selected`, `Paper is Loaded` and `Printer OK`.
    - If your status differs from the one above, use `emulatedprinter -needed_option` to set the correct status
    - Unload modules using (`make stop`) or (`rmmod g_printer` and `rmmod usb_f_printer`)
 
# Emulate the Printer
  Before you can start emulating your printer you will need to find out
  some information about your exact printer model. This includes the manufacturer,
  the model and the IEEE1284/PNP string. These are the minimum requieremnts for Microsoft
  Windows to load the correct driver. 

  Manufacturer & Model: 
  Typically you already know this. Manufacturer might be 
  something like HP, Brother, Xerox, ... and Model might be DeskJet, DocuPrint, ... 
  Now you will need look up both the vendorID and the productID assigned be the USB-IF.
  You will find multiple lists online listing both vendorIDs and productIDs. [Here is an
  example](https://www.the-sz.com/products/usbid/).

  IEEE1284 / PNP string:
  This is where it will get a little more tricky. The IEEE1284 string is send
  by your printer to your PC whenever you are connecting the printer to your PC.
  You need to use a USB traffic analyzer to capture the string. The USBlyzer Team
  offers a great solution called USBlyzer. You will need to start the traffic analyzer,
  capture hot-plugged devices and then connect your printer. During the enumeration
  the printer will send the PNP string. When you are already here, you can confirm
  that you looked up the correct vendorID and productID. Just search through the
  descriptors.

  Once you have the requiered data you can load the g_printer module:
  `modprobe g_printer idVendor=0x1234 idProduct=0x1234 pnp_string="MFG:;CMD:;CLS:PRINTER;"`
  where idVendor, idProduct and pnp_string have your values. You can find all parameters
  including a great explaination [here](https://www.kernel.org/doc/Documentation/usb/gadget_printer.rst).
  If you want to, you can now add the modprobe command to run at boot time.

  If you choose to emulate a different printer than your own, you might run into problems
  getting the information about your exact device. Luckily there is an awesome website
  called [www.openprinting.org](http://www.openprinting.org/foomatic-db/db/source/printer/) where you will find
  almost 4000 (!) printers listed together with their IEEE1284 string. Once you have a printer 
  that fits your needs and has a pnp string, you can simply use the same online list as before to get
  the vendorID and productID. As before load the g_printer module and confirm that 
  your PC recocnizes the Pi correctly as the printer you selected.

  Now simply call `emulatedprinter -read_data` to capture all print jobs and
  save them to the folder you specified earlier. 
      
# Acknowledgements    
This program is made possible due to the work of David Brownell and 
Craig W. Nadler who added the USB printer gadget to the 
Linux USB Gadget API. 
[Here you can find a good explaination of how the g_printer module 
can be used!](https://www.kernel.org/doc/Documentation/usb/gadget_printer.rst)
We also used these examples as a basis for this program.
      
   
   
