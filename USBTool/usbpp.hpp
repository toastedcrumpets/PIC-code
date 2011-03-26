#include <libusb-1.0/libusb.h>
#include <vector>
#include <stdexcept>
#include <algorithm>

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
  
  const libusb_interface& getInterface(size_t i) const
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
  friend class USBDeviceHandle;
  libusb_device* _dev;

  void incref(libusb_device* dev) { _dev = libusb_ref_device(dev); }
};

class USBDeviceHandle
{
public:
  USBDeviceHandle(USBDevice& dev)
  {
    if (libusb_open(dev._dev, &_devHandle))
      throw std::runtime_error("Failed to open the device");
  }

  ~USBDeviceHandle()
  {
    //Release the claimed interfaces
    for (std::vector<int>::iterator iPtr = _claimedInterfaces.begin();
	 iPtr != _claimedInterfaces.end(); ++iPtr)
      libusb_release_interface(_devHandle, *iPtr);

    //Return them to the kernel if required
    for (std::vector<int>::iterator iPtr = _kernelClaimedInterfaces.begin();
	 iPtr != _kernelClaimedInterfaces.end(); ++iPtr)
      libusb_attach_kernel_driver(_devHandle, *iPtr);

    libusb_close(_devHandle);
  }

  void claimInterface(int interface)
  {
    //Check if the interface is already claimed!
    if (std::find(_claimedInterfaces.begin(), _claimedInterfaces.end(), interface) 
	!= _claimedInterfaces.end())
      return;

    int r = libusb_kernel_driver_active(_devHandle, interface);

    if ((r != 0) && (r != 1))
      throw std::runtime_error("Failed to test if the kernel driver was loaded for an interface");

    if (r == 1)
      {
	libusb_detach_kernel_driver(_devHandle, interface);
	_kernelClaimedInterfaces.push_back(interface);
      }

    if (libusb_claim_interface(_devHandle, interface))
      throw std::runtime_error("Failed to claim the interface");
    _claimedInterfaces.push_back(interface);
  }

private:
  libusb_device_handle *_devHandle;
  std::vector<int> _claimedInterfaces;
  std::vector<int> _kernelClaimedInterfaces;
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

