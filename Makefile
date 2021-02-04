all:
	@echo
	@echo [!] Please specify target
	@echo
build:
	gcc -Wall printer.c -o emulatedprinter

clean:
	rm -f printjobs/*.pcl
	rm -f printjobs/*.pdf
	rm -f printjobs/*.txt


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
startHP1220:
	modprobe g_printer idVendor=0x03F0 idProduct=0x0412 bcdDevice=0x0100 iManufacturer="" iProduct="" iSerialNumber="" iPNPstring="MFG:Hewlett-Packard;CMD:PJL,MLC,PCL,PCLXL,POSTSCRIPT;MDL:HP LaserJet 1220;CLS:PRINTER;DES:Hewlett-Packard LaserJet 1220;"
startXeroxDocuPrintCM215:
	modprobe g_printer idVendor=0x0550 idProduct=0x0165 bcdDevice=0x0100 iManufacturer="" iProduct="" iSerialNumber="" iPNPstring="MFG:FUJI XEROX;CMD:HBPL;MDL:DocuPrint CM215 b;CLS:PRINTER;  "
startXeroxPhaser4400:
	modprobe g_printer idVendor=0x0924 idProduct=0x3CE8 bcdDevice=0x0100 iManufacturer="" iProduct="" iSerialNumber="" iPNPstring="MFG:Xerox;CMD:Adobe PostScript 3, PCL, PJL;MDL:Phaser 4400B;CLS:Printer;DES:Xerox Phaser 4400 Laser Printer, Page Printer, PostScript 3, Letter/A4 Size;  "
startSamsungML6060:
	modprobe g_printer idVendor=0x04E8 idProduct=0x3008 bcdDevice=0x0100 iManufacturer="" iProduct="" iSerialNumber="" iPNPstring="MFG:Samsung;CMD:PCL5E,PCL6;MDL:ML-6060;CLS:PRINTER;CMT:EmulatedPrinter;  "
startRicohAficio1224C:
	modprobe g_printer idVendor=0x05CA idProduct=0x0403 bcdDevice=0x0100 iManufacturer="" iProduct="" iSerialNumber="" iPNPstring="MFG:RICOH;MDL:Aficio 1224C;CMD:POSTSCRIPT;  "
startKyoceraFS1020D:
	modprobe g_printer idVendor=0x0482 idProduct=0x0015 bcdDevice=0x0100 iManufacturer="" iProduct="" iSerialNumber="" iPNPstring="MFG:Kyocera;MDL:Kyocera FS-1030D;CMD:POSTSCRIPT,PJL,PCL;  "
startRicohAficioSP35010SF:
	modprobe g_printer idVendor=0x05CA idProduct=0x0437 bcdDevice=0x0100 iManufacturer="" iProduct="" iSerialNumber="" iPNPstring="MFG:RICOH;MDL:Aficio SP 3510SF;CMD:POSTSCRIPT;  "
