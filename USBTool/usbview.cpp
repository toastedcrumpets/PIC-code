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

      for (size_t i(0); i < desc.getConfigCount(); ++i)
	{
	  USBConfigDescriptor cdesc = iPtr->getConfigDescriptor(i);
	  std::cout << "\n  Interfaces " << cdesc.getInterfaceCount()
	    ;
	}

      std::cout << "\n";
    }

  return 0;
}
