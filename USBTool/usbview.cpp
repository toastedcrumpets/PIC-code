#include <iostream>
#include "usbpp.hpp"

int main(int argc, char *argv[])
{
  USB usbstate;
  
  usbstate.setDebug();
  
  std::vector<USBDevice> devices = usbstate.getDevices();

  for (std::vector<USBDevice>::const_iterator iPtr = devices.begin();
       iPtr != devices.end(); ++iPtr)
    {
      USBDeviceDescriptor desc = iPtr->getDeviceDescriptor();
      std::cout << "Device Class " << (int)desc.getDeviceClass()
		<< "\n Vendor ID " << desc.getVendorID()
		<< "\n Product ID " << desc.getProductID()
		<< "\n Configurations " << desc.getConfigCount();

      for (size_t c(0); c < desc.getConfigCount(); ++c)
	{
	  USBConfigDescriptor cdesc = iPtr->getConfigDescriptor(c);
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

  return 0;
}
