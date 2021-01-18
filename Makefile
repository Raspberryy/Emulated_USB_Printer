all:
	@echo
	@echo [!] Please specify target
	@echo
build:
	gcc -Wall printer.c -o emulatedprinter

# Stops currently running printer
# Throws error if no printer is running
stop:
	rmmod g_printer
	rmmod usb_f_printer

# Printer models
startHPCXI895:
	modprobe g_printer idVendor=0x03F0 idProduct=0x0004 bcdDevice=0x0100 iManufacturer="" iProduct="" iSerialNumber="" iPNPstring="MFG:HEWLETT-PACKARD;MDL:DESKJET 895C;CMD:MLC,PCL,PML;CLASS:PRINTER;DESCRIPTION:Hewlett-Packard DeskJet 895C;VSTATUS:$HB0$FC0,ff,DN,IDLE,CUT;"
startBrotherHL2030:
	modprobe g_printer idVendor=0x04F9 idProduct=0x0027 bcdDevice=0x0100 iManufacturer="" iProduct="" iSerialNumber="" iPNPstring="MFG:Brother;CMD:PJL,HBP;MDL:HL-2030 series;CLS:PRINTER;"
