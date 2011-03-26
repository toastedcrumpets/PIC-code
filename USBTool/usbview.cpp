#include <iostream>
#include <vector>
#include <stdexcept>
#include <libusb-1.0/libusb.h>


class USBDeviceDescriptor
{
public:
  size_t getConfigCount() const { return _desc.bNumConfigurations; }
  uint8_t getDeviceClass() const { return _desc.bDeviceClass; }
  uint16_t getVendorID() const { return _desc.idVendor; }
  uint16_t getProductID() const { return _desc.idProduct; }
private:  
  friend class USBDevice;

  USBDeviceDescriptor() {}

  libusb_device_descriptor _desc;
};

class USBConfigDescriptor
{
public:
  ~USBConfigDescriptor() 
  { libusb_free_config_descriptor(_desc); }

  size_t getInterfaceCount() const { return _desc->bNumInterfaces; }
  
  const libusb_interface getInterface(size_t i)
  { return _desc->interface[i]; }

private:  
  friend class USBDevice;

  USBConfigDescriptor() {}

  libusb_config_descriptor* _desc;
};

class USBDevice
{
public:
  //Constructors/Destructors
  USBDevice():_dev(NULL) {}

  USBDevice(libusb_device* dev) { incref(dev); }

  USBDevice(const USBDevice& ud):_dev(NULL) 
  { if (ud._dev != NULL) incref(ud._dev); }

  USBDevice& operator=(const USBDevice& ud)
  { 
    _dev = NULL;
    if (ud._dev != NULL) incref(ud._dev);
    return *this;
  }

  ~USBDevice() { if (_dev != NULL) libusb_unref_device(_dev); }

  //Get data
  USBDeviceDescriptor getDeviceDescriptor() const
  {
    USBDeviceDescriptor desc;    
    if (libusb_get_device_descriptor(_dev, &desc._desc))
      throw std::runtime_error("libUSB device descriptor error");
    return desc;
  }
  
  USBConfigDescriptor getConfigDescriptor(uint8_t i) const
  {
    USBConfigDescriptor desc;    
    if (libusb_get_config_descriptor(_dev, i, &(desc._desc)))
      throw std::runtime_error("libUSB config descriptor error");
    return desc;
  }

private:
  libusb_device* _dev;

  void incref(libusb_device* dev) { _dev = libusb_ref_device(dev); }
};


class USB
{
public:
  USB() 
  {
    if (libusb_init(&_context) < 0) 
      throw std::runtime_error("libUSB init error"); 
  }

  ~USB() { libusb_exit(_context); }

  void setDebug(int lvl = 3) { libusb_set_debug(_context, lvl); }

  std::vector<USBDevice> getDevices()
  {
    libusb_device** devs;
    size_t count = libusb_get_device_list(_context, &devs);
    if (count < 0)
      throw std::runtime_error("libUSB getDevices error");

    std::vector<USBDevice> retval;

    for (size_t i(0); i < count; ++i)
      retval.push_back(USBDevice(devs[i]));

    libusb_free_device_list(devs, 1);

    return retval;
  }

private:
  libusb_context* _context;
};


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
		    std::cout << "\n     DescriptorType: " << (int)cdesc.getInterface(i).altsetting[a].endpoint[e].bDescriptorType
			      << "\n     EP Address: " << (int)cdesc.getInterface(i).altsetting[a].endpoint[e].bDescriptorType ;
		}
	    }
	}

      std::cout << "\n";
    }

  return 0;
}
