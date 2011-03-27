#include <iostream>
#include "usbpp.hpp"
#include <memory>

int main(int argc, char *argv[])
{
  try {
    USB usbstate;
  
    usbstate.setDebug();
  
    std::vector<USBDevice> devices = usbstate.getDevices();

    USBDevice dev;
    for (std::vector<USBDevice>::const_iterator iPtr = devices.begin();
	 iPtr != devices.end(); ++iPtr)
      {
	USBDeviceDescriptor desc = iPtr->getDeviceDescriptor();
	if ((desc.getDeviceClass() == 0x00)
	    && (desc.getVendorID() == 0x04D8)	
	    && (desc.getProductID() == 0x0204))
	  {
	    dev = *iPtr;
	    break;
	  }
      }
    
    if (!dev)
      {
	std::cout << "\nCouldn't find USB device!";
	return 1;
      }

    std::cout << "\nFound USB device!";
    
    USBDeviceHandle devhandle(dev);

    devhandle.setConfiguration(1);
    devhandle.claimInterface(0);

    std::cout << "\nClaimed interface, press enter to toggle LED";
    char a = '0';
    while (a != '\n')     
      std::cin.get(a);

    {
      std::vector<unsigned char> OutputPacketBuffer(64);
      OutputPacketBuffer[0] = 0x80;
      devhandle.syncBulkTransfer(0x01, OutputPacketBuffer, 5000);
    }

    std::cout << "\nToggled LED, press enter to toggle again";
    a = '0';
    while (a != '\n')     
      std::cin.get(a);    

    {
      std::vector<unsigned char> OutputPacketBuffer(64);
      OutputPacketBuffer[0] = 0x80;
      devhandle.syncBulkTransfer(0x01, OutputPacketBuffer, 5000);
    }

    std::cout << "\nToggled LED";
    
    while (1)
      {
	std::cout << "\nPress enter to read the button status";
	a = '0';
	while (a != '\n')
	  std::cin.get(a);
	
	{
	  //First request the status read
	  std::vector<unsigned char> OutputPacketBuffer(64);
	  OutputPacketBuffer[0] = 0x81;
	  devhandle.syncBulkTransfer(0x01, OutputPacketBuffer, 5000);
	  std::cout << "\nSent request packet" << std::endl;

	  //Now read the status packet
	  std::vector<unsigned char> InputPacketBuffer(64);
	  devhandle.syncBulkTransfer(0x81, InputPacketBuffer, 5000);
	  
	  if (InputPacketBuffer[1] == 0x01)
	    std::cout << "\nButton is up!";
	  else
	    std::cout << "\nButton is down!";
	}
      }
    
  }
  catch (std::exception& err)
    {
      std::cout.flush();
      std::cout << "\nException caught in main(): what() " << err.what();
      return 1;
    }

  std::cout << "\n";
  return 0;
}
