#include <libusb-1.0/libusb.h>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace USB
{
  enum EndPointDirection {
    IN    = 0x80,
    OUT   = 0x00,
  };

  class DeviceDescriptor
  {
  public:
    size_t getConfigCount() const { return _desc.bNumConfigurations; }
    uint8_t getDeviceClass() const { return _desc.bDeviceClass; }
    uint16_t getVendorID() const { return _desc.idVendor; }
    uint16_t getProductID() const { return _desc.idProduct; }
    uint8_t getManufacturerStrIdx() const { return _desc.iManufacturer; }
    uint8_t getProductStrIdx() const { return _desc.iProduct; }
    uint8_t getSerialStrIdx() const { return _desc.iSerialNumber; }
  private:  
    friend class Device;

    DeviceDescriptor() {}

    libusb_device_descriptor _desc;
  };

  class ConfigDescriptor
  {
  public:
    ~ConfigDescriptor() 
    { libusb_free_config_descriptor(_desc); }

    size_t getInterfaceCount() const { return _desc->bNumInterfaces; }
  
    const libusb_interface& getInterface(size_t i) const
    { return _desc->interface[i]; }

  private:  
    friend class Device;

    ConfigDescriptor() {}

    libusb_config_descriptor* _desc;
  };


  class Device
  {
  public:
    //Constructors/Destructors
    Device():_dev(NULL) {}

    Device(libusb_device* dev) { incref(dev); }

    Device(const Device& ud):_dev(NULL) 
    { if (ud._dev != NULL) incref(ud._dev); }

    Device& operator=(const Device& ud)
    { 
      _dev = NULL;
      if (ud._dev != NULL) incref(ud._dev);
      return *this;
    }

    operator bool() const {return _dev != NULL; }

    ~Device() { if (_dev != NULL) libusb_unref_device(_dev); }

    //Get data
    DeviceDescriptor getDeviceDescriptor() const
    {
      DeviceDescriptor desc;    
      if (libusb_get_device_descriptor(_dev, &desc._desc))
	throw std::runtime_error("libUSB device descriptor error");
      return desc;
    }
  
    ConfigDescriptor getConfigDescriptor(uint8_t i) const
    {
      ConfigDescriptor desc;    
      if (libusb_get_config_descriptor(_dev, i, &(desc._desc)))
	throw std::runtime_error("libUSB config descriptor error");
      return desc;
    }

    uint8_t getBusNumber() const
    { return libusb_get_bus_number(_dev); }

    uint8_t getDeviceAddress() const
    { return libusb_get_device_address(_dev); }
  
  private:
    friend class DeviceHandle;
    libusb_device* _dev;

    void incref(libusb_device* dev) { _dev = libusb_ref_device(dev); }
  };

  class DeviceHandle
  {
  public:
    DeviceHandle(const Device& dev)
    {
      if (!dev)
	throw std::runtime_error("Invalid device passed to DeviceHandle Constructor");

      if (libusb_open(dev._dev, &_devHandle))
	throw std::runtime_error("Failed to open the device");
    }

    ~DeviceHandle()
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

    void setConfiguration(int config)
    {
      if (libusb_set_configuration(_devHandle, config))
	throw std::runtime_error("Failed to set configuration");
    }

    uint16_t getLanguageCode(size_t i=0)
    {
      std::vector<unsigned char> data = getStringDescriptorRaw(0, 0);
      //data contains the following information
      //Byte Offset     Details
      //   0            Length of descriptor in bytes
      //   1            Special 0x03 marker for string descriptor
      // 2-3            Language 0
      // 4-5            Language 1
      // ...

      if (2 * (i + 1) + 1 >= data.size())
	throw std::runtime_error("getLanguageCode index out of range");

      //Read the requested language value
      return (data[2 * (i + 1)] << 8) | data[2 * (i + 1) + 1];
    }

    std::wstring getStringDescriptor(uint8_t desc_index, uint16_t lang_index)
    {
      std::vector<unsigned char> data = getStringDescriptorRaw(desc_index, lang_index);
    
      std::wstring retval;
      retval.resize((data.size() - 2) / 2);
      for (size_t i(0); i < (data.size() - 2) / 2; ++i)
	retval[i] = *reinterpret_cast<uint16_t*>(&data[2*i+2]);
      return retval;
    }

    int syncBulkTransfer(unsigned int endpoint, std::vector<unsigned char>& data,
			 unsigned int timeout = 0)
    {
      int bytesTransfered;
      if (libusb_bulk_transfer(_devHandle, endpoint, &data[0], data.size(), 
			       &bytesTransfered, timeout))
	throw std::runtime_error("Synchronous bulk transfer failed");
      return bytesTransfered;
    }

  private:
    //Cannot copy device handles!
    DeviceHandle(const DeviceHandle& dev);

    std::vector<unsigned char> getStringDescriptorRaw(uint8_t desc_index, uint16_t lang_index)
    {
      std::vector<unsigned char> data(1);
      //We only read the first byte, just to get the size of the whole packet
      int bytes_written = libusb_get_string_descriptor(_devHandle, desc_index, lang_index, &data[0], 1);
    
      if (bytes_written < 0)
	throw std::runtime_error("Failed to obtain string descriptor size");
    
      if (data[0] < 0)
	throw std::runtime_error("String descriptor has negative size");      

      int packetSize = data[0];

      //Now we can request the whole packet
      data.resize(packetSize);
      bytes_written = libusb_get_string_descriptor(_devHandle, desc_index, lang_index, 
						   &data[0], packetSize);
    
      if (bytes_written < 0)
	throw std::runtime_error("Failed to obtain string descriptor");

      if (data[0] != bytes_written)
	throw std::runtime_error("Bytes written and packet size mismatch");

      return data;
    }

    libusb_device_handle *_devHandle;
    std::vector<int> _claimedInterfaces;
    std::vector<int> _kernelClaimedInterfaces;
  };

  class Context
  {
  public:
    Context() 
    {
      if (libusb_init(&_context) < 0) 
	throw std::runtime_error("libUSB init error"); 
    }

    ~Context() { libusb_exit(_context); }

    void setDebug(int lvl = 3) { libusb_set_debug(_context, lvl); }

    std::vector<Device> getDevices()
    {
      libusb_device** devs;
      size_t count = libusb_get_device_list(_context, &devs);
      if (count < 0)
	throw std::runtime_error("libUSB getDevices error");

      std::vector<Device> retval;

      for (size_t i(0); i < count; ++i)
	retval.push_back(Device(devs[i]));

      libusb_free_device_list(devs, 1);

      return retval;
    }

  private:
    libusb_context* _context;
  };
}
