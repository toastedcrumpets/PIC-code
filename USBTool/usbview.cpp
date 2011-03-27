#include <iostream>
#include "usbpp.hpp"
#include <memory>

int main(int argc, char *argv[])
{
  try {
    USB::Context usbstate;
  
    usbstate.setDebug();
  
    std::vector<USB::Device> devices = usbstate.getDevices();

    for (std::vector<USB::Device>::const_iterator iPtr = devices.begin();
	 iPtr != devices.end(); ++iPtr)
      {
	USB::DeviceHandle handle(*iPtr);
	USB::DeviceDescriptor desc = iPtr->getDeviceDescriptor();
	std::cout << "Device Class " << (int)desc.getDeviceClass()
		  << ", BusNo. " << (int)iPtr->getBusNumber()
		  << ", Address " << (int)iPtr->getDeviceAddress()
		  << "\n Vendor ID " << desc.getVendorID()
		  << "\n Product ID " << desc.getProductID()
		  << "\n Configurations " << desc.getConfigCount();

	try {
	  std::wcout << "\n Language Code " << handle.getLanguageCode()
		     << "\n Manufacturer " << handle.getStringDescriptor(desc.getManufacturerStrIdx(), 
									 handle.getLanguageCode())
		     << "\n Product " << handle.getStringDescriptor(desc.getProductStrIdx(), 
								    handle.getLanguageCode())
		     << "\n Serial No." << handle.getStringDescriptor(desc.getSerialStrIdx(), 
								      handle.getLanguageCode());
	} catch (std::exception)
	  {}

	for (size_t c(0); c < desc.getConfigCount(); ++c)
	  {
	    USB::ConfigDescriptor cdesc = iPtr->getConfigDescriptor(c);
	    std::cout << "\n  Interfaces " << cdesc.getInterfaceCount();

	    for (size_t i(0); i < cdesc.getInterfaceCount(); ++i)
	      {
		std::cout << "\n   No. of alternate settings " 
			  << cdesc.getInterface(i).num_altsetting;
		for (size_t a(0); a < (size_t)cdesc.getInterface(i).num_altsetting; ++a)
		  {
		    std::cout << "\n    Interface Number:" 
			      << (int)cdesc.getInterface(i).altsetting[a].bInterfaceNumber
			      << "\n    Endpoints:" 
			      << (int)cdesc.getInterface(i).altsetting[a].bNumEndpoints;
		  
		    for (size_t e(0); e < (size_t)cdesc.getInterface(i).altsetting[a].bNumEndpoints; ++e)
		      std::cout << "\n     DescriptorType: " 
				<< (int)cdesc.getInterface(i).altsetting[a].endpoint[e].bDescriptorType
				<< "\n     EP Address: " 
				<< (int)cdesc.getInterface(i).altsetting[a].endpoint[e].bDescriptorType ;
		  }
	      }
	  }

	std::cout << "\n";
      }
  }
  catch (std::exception& err)
    {
      std::cout.flush();
      std::cout << "\nException caught in main(): what() " << err.what();
      return 1;
    }
  return 0;
}
