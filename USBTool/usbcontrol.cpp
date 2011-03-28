#include <iostream>
#include "usbpp.hpp"
#include <memory>

void pause(std::string message)
{
  std::cout << "\n" << message << " Press enter to continue:";
  char a = '0';
  while (a != '\n') std::cin.get(a);
}

void bandwidthTest(unsigned char mode, USB::DeviceHandle& devhandle)
{
    std::vector<unsigned char> buffer(64);
    timespec acc_tstartTime, acc_tendTime;
    
    size_t packetsSent = 2000;
    buffer[0] = mode;

    clock_gettime(CLOCK_MONOTONIC, &acc_tstartTime);
    for (size_t i(0); i < packetsSent; ++i)
      {
	//Write it out
	devhandle.syncBulkTransfer(1 | USB::OUT, buffer, 5000);
	//Read it back
	devhandle.syncBulkTransfer(1 | USB::IN, buffer, 5000);
      }
    
    clock_gettime(CLOCK_MONOTONIC, &acc_tendTime);
    double bytesPerSec = packetsSent * 64
      / (double(acc_tendTime.tv_sec) + 1e-9 * double(acc_tendTime.tv_nsec)
	 - double(acc_tstartTime.tv_sec) - 1e-9 * double(acc_tstartTime.tv_nsec));

    std::cout << "\nManaged to read (and write) at " << bytesPerSec / 1024 << " kB / s";
}

int main(int argc, char *argv[])
{
  try {
    USB::Context usbstate;
  
    std::vector<USB::Device> devices = usbstate.getDevices();

    USB::Device dev;
    for (std::vector<USB::Device>::const_iterator iPtr = devices.begin();
	 iPtr != devices.end(); ++iPtr)
      {
	USB::DeviceDescriptor desc = iPtr->getDeviceDescriptor();
	if ((desc.getVendorID() == 0x04D8) && (desc.getProductID() == 0x0204))
	  { dev = *iPtr; break; }
      }
    
    if (!dev)
      {
	std::cout << "\nCouldn't find USB device!";
	return 1;
      }

    std::cout << "\nFound USB device!";
    
    //Initialising the device
    USB::DeviceHandle devhandle(dev);
    devhandle.setConfiguration(1);
    devhandle.claimInterface(0);

    std::vector<unsigned char> buffer(64);

    pause("Claimed Interface, toggling LED");
    buffer[0] = 0x80;
    devhandle.syncBulkTransfer(1 | USB::OUT, buffer, 5000);

    pause("LED toggled once, toggling again");
    devhandle.syncBulkTransfer(1 | USB::OUT, buffer, 5000);

    std::cout << "\nToggled LED";

    pause("Performing writeback bandwidth test, with PIC buffer-buffer copy");
    bandwidthTest(0x82, devhandle);

    pause("Performing writeback bandwidth test, without copy");
    bandwidthTest(0x83, devhandle);

    std::cout << "\n";
    double updaterate = 0;

    timespec acc_tstartTime;
    while (1)
      {
	clock_gettime(CLOCK_MONOTONIC, &acc_tstartTime);
	
	const size_t loops = 1000;
	for (size_t i(0); i < loops; ++i)
	  {
	    //First request the status read
	    buffer[0] = 0x81;
	    devhandle.syncBulkTransfer(1 | USB::OUT, buffer, 5000);
	    
	    //Now read the status packet
	    devhandle.syncBulkTransfer(1 | USB::IN, buffer, 5000);
	    
	    std::cout << "\rButton is " << ((buffer[1] == 0x01) ? "UP  " : "DOWN" )
		      << " AD val is " << *((uint16_t*)(&buffer[2]))
		      << " updates/s = " << updaterate
		      << "     ";
	    
	    std::cout.flush();
	  }
	timespec acc_tendTime;
	clock_gettime(CLOCK_MONOTONIC, &acc_tendTime);
	updaterate = loops
	  / (double(acc_tendTime.tv_sec) + 1e-9 * double(acc_tendTime.tv_nsec)
	     - double(acc_tstartTime.tv_sec) - 1e-9 * double(acc_tstartTime.tv_nsec));
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
