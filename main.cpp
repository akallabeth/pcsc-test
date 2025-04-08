#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string.h>
#include <winscard.h>

#define SCARD_ATTR_VALUE(Class, Tag) ((((ULONG)(Class)) << 16) | ((ULONG)(Tag)))

#define SCARD_CLASS_VENDOR_INFO 1
#define SCARD_ATTR_VENDOR_NAME SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0100)
#define SCARD_ATTR_VENDOR_IFD_TYPE                                             \
  SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0101)
#define SCARD_ATTR_VENDOR_IFD_VERSION                                          \
  SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0102)
#define SCARD_ATTR_VENDOR_IFD_SERIAL_NO                                        \
  SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0103)

#if defined(_WIN32)
#define SCARD_PROTOCOL_ANY (SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1)

static const char *pcsc_stringify_error(const LONG val) {
#define CASE_STR(x)                                                            \
  case x:                                                                      \
    return #x;

  switch (val) {
    CASE_STR(SCARD_S_SUCCESS)
    CASE_STR(SCARD_F_INTERNAL_ERROR)
    CASE_STR(SCARD_E_CANCELLED)
    CASE_STR(SCARD_E_INVALID_HANDLE)
    CASE_STR(SCARD_E_INVALID_PARAMETER)
    CASE_STR(SCARD_E_INVALID_TARGET)
    CASE_STR(SCARD_E_NO_MEMORY)
    CASE_STR(SCARD_F_WAITED_TOO_LONG)
    CASE_STR(SCARD_E_INSUFFICIENT_BUFFER)
    CASE_STR(SCARD_E_UNKNOWN_READER)
    CASE_STR(SCARD_E_TIMEOUT)
    CASE_STR(SCARD_E_SHARING_VIOLATION)
    CASE_STR(SCARD_E_NO_SMARTCARD)
    CASE_STR(SCARD_E_UNKNOWN_CARD)
    CASE_STR(SCARD_E_CANT_DISPOSE)
    CASE_STR(SCARD_E_PROTO_MISMATCH)
    CASE_STR(SCARD_E_NOT_READY)
    CASE_STR(SCARD_E_INVALID_VALUE)
    CASE_STR(SCARD_E_SYSTEM_CANCELLED)
    CASE_STR(SCARD_F_COMM_ERROR)
    CASE_STR(SCARD_F_UNKNOWN_ERROR)
    CASE_STR(SCARD_E_INVALID_ATR)
    CASE_STR(SCARD_E_NOT_TRANSACTED)
    CASE_STR(SCARD_E_READER_UNAVAILABLE)
    CASE_STR(SCARD_P_SHUTDOWN)
    CASE_STR(SCARD_E_PCI_TOO_SMALL)
    CASE_STR(SCARD_E_READER_UNSUPPORTED)
    CASE_STR(SCARD_E_DUPLICATE_READER)
    CASE_STR(SCARD_E_CARD_UNSUPPORTED)
    CASE_STR(SCARD_E_NO_SERVICE)
    CASE_STR(SCARD_E_SERVICE_STOPPED)
    CASE_STR(SCARD_E_ICC_INSTALLATION)
    CASE_STR(SCARD_E_ICC_CREATEORDER)
    CASE_STR(SCARD_E_UNSUPPORTED_FEATURE)
    CASE_STR(SCARD_E_DIR_NOT_FOUND)
    CASE_STR(SCARD_E_FILE_NOT_FOUND)
    CASE_STR(SCARD_E_NO_DIR)
    CASE_STR(SCARD_E_NO_FILE)
    CASE_STR(SCARD_E_NO_ACCESS)
    CASE_STR(SCARD_E_WRITE_TOO_MANY)
    CASE_STR(SCARD_E_BAD_SEEK)
    CASE_STR(SCARD_E_INVALID_CHV)
    CASE_STR(SCARD_E_UNKNOWN_RES_MNG)
    CASE_STR(SCARD_E_NO_SUCH_CERTIFICATE)
    CASE_STR(SCARD_E_CERTIFICATE_UNAVAILABLE)
    CASE_STR(SCARD_E_NO_READERS_AVAILABLE)
    CASE_STR(SCARD_E_COMM_DATA_LOST)
    CASE_STR(SCARD_E_NO_KEY_CONTAINER)
    CASE_STR(SCARD_E_SERVER_TOO_BUSY)
    CASE_STR(SCARD_W_UNSUPPORTED_CARD)
    CASE_STR(SCARD_W_UNRESPONSIVE_CARD)
    CASE_STR(SCARD_W_UNPOWERED_CARD)
    CASE_STR(SCARD_W_RESET_CARD)
    CASE_STR(SCARD_W_REMOVED_CARD)
    CASE_STR(SCARD_W_SECURITY_VIOLATION)
    CASE_STR(SCARD_W_WRONG_CHV)
    CASE_STR(SCARD_W_CHV_BLOCKED)
    CASE_STR(SCARD_W_EOF)
    CASE_STR(SCARD_W_CANCELLED_BY_USER)
    CASE_STR(SCARD_W_CARD_NOT_AUTHENTICATED)
  default:
    return "foobar";
  }
}
#endif

static std::string string_to_hex(const std::string& input)
{
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2 + 3);
    output.push_back('0');
    output.push_back('x');
    for (unsigned char c : input)
    {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}

static std::string err2str(LONG val) {
  std::stringstream ss;
  ss << pcsc_stringify_error(val) << "[0x" << std::hex << std::setfill('0')
     << std::setw(8) << val << "]";
  return ss.str();
}

class cardhandle {
public:
  cardhandle(SCARDCONTEXT context, const std::string &name,
             bool autoallocate = false)
      : _context(context), _name(name), _autoalloc(autoallocate) {
    DWORD active = 0;
    auto rc = SCardConnect(context, name.c_str(), SCARD_SHARE_SHARED,
                           SCARD_PROTOCOL_ANY, &_card, &active);
    std::cout << "SCardConnect: (autoalloc " << _autoalloc << ") " << name
              << ": " << err2str(rc) << std::endl;
    if (rc != SCARD_S_SUCCESS)
      throw rc;
  }
    ~cardhandle() {
        auto rc = SCardDisconnect(_card, 0);
        std::cout << "SCardDisconnect: (autoalloc " << _autoalloc << ") "
                  << err2str(rc) << std::endl;
    }

    bool list() const {
        const std::map<DWORD, std::string> attrs = {
            {SCARD_ATTR_VENDOR_NAME, "SCARD_ATTR_VENDOR_NAME"},
            {SCARD_ATTR_VENDOR_IFD_TYPE, "SCARD_ATTR_VENDOR_IFD_TYPE"},
            {SCARD_ATTR_VENDOR_IFD_VERSION, "SCARD_ATTR_VENDOR_IFD_VERSION"},
            {SCARD_ATTR_VENDOR_IFD_SERIAL_NO, "SCARD_ATTR_VENDOR_IFD_SERIAL_NO"}
        };

        for (auto attr : attrs) {
            DWORD len = 0;
            std::string sattr;
            if (_autoalloc) {
              len = SCARD_AUTOALLOCATE;
              char *str = nullptr;
              auto rc = SCardGetAttrib(_card, attr.first,
                                       reinterpret_cast<LPBYTE>(&str), &len);
              std::cout << "SCardGetAttrib: (1): (autoalloc " << _autoalloc
                        << ") " << attr.second << ": " << err2str(rc)
                        << std::endl;
              if (rc != SCARD_S_SUCCESS)
                continue;

              sattr = std::string(str, len);
              SCardFreeMemory(_context, str);
            } else {
              auto rc = SCardGetAttrib(_card, attr.first, nullptr, &len);
              std::cout << "SCardGetAttrib: (1): (autoalloc " << _autoalloc
                        << ") " << attr.second << ": " << err2str(rc)
                        << std::endl;
              if (rc != SCARD_S_SUCCESS)
                continue;

            sattr.resize(len);

            auto rc2 = SCardGetAttrib(_card, attr.first, reinterpret_cast<LPBYTE>(sattr.data()), &len);
            std::cout << "SCardGetAttrib: (2): (autoalloc " << _autoalloc
                      << ") " << rc2 << std::endl;
            if (rc2 != SCARD_S_SUCCESS)
                throw rc2;
            }
            std::cout << "SCardGetAttrib: (2): (autoalloc " << _autoalloc
                      << ") " << attr.second << ": [" << len << "] {"
                      << string_to_hex(sattr) << "} '" << sattr.c_str() << "'"
                      << std::endl;
        }
        return true;
    }

    private:
      SCARDCONTEXT _context;
      SCARDHANDLE _card;
      std::string _name;
      bool _autoalloc = false;
};

class connection {
public:
  connection(bool autoalloc = false) : _autoalloc(autoalloc) {
    auto rc =
        SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr, nullptr, &_context);
    std::cout << "SCardEstablishContext: (autoalloc " << _autoalloc << ") "
              << err2str(rc) << std::endl;
    if (rc != SCARD_S_SUCCESS)
      throw rc;
  }

    ~connection() {
        auto rc = SCardReleaseContext(_context);
        std::cout << "SCardReleaseContext: (autoalloc " << _autoalloc << ") "
                  << err2str(rc) << std::endl;
    }

    bool valid() const {
        auto rc = SCardIsValidContext(_context);
        return rc == SCARD_S_SUCCESS;
    }

    bool for_each() {
      DWORD count = 0;
      std::string list;

      if (_autoalloc) {
        count = SCARD_AUTOALLOCATE;
        char *str = nullptr;
        auto rc = SCardListReaders(_context, nullptr,
                                   reinterpret_cast<char *>(&str), &count);
        std::cout << "SCardListReaders: (autoalloc " << _autoalloc << ") "
                  << err2str(rc) << std::endl;
        if (rc != SCARD_S_SUCCESS)
          throw rc;

        list = std::string(str, count);
        SCardFreeMemory(_context, str);
      } else {
        auto rc = SCardListReaders(_context, nullptr, nullptr, &count);
        std::cout << "SCardListReaders: (autoalloc " << _autoalloc << ") "
                  << err2str(rc) << std::endl;
        if (rc != SCARD_S_SUCCESS)
          throw rc;

        list.resize(count);
        auto rc2 = SCardListReaders(_context, nullptr, list.data(), &count);
        std::cout << "SCardListReaders: (autoalloc " << _autoalloc << ") "
                  << err2str(rc) << std::endl;
        if (rc2 != SCARD_S_SUCCESS)
          throw rc2;
      }

      size_t pos = 0;
      while (pos < count) {
        const char *str = &list.c_str()[pos];
        const size_t len = strnlen(str, count);
        pos += len + 1;
        if (len > 0) {
          std::cout << "reader: (autoalloc " << _autoalloc << ") " << str
                    << std::endl;
          cardhandle card(_context, str, _autoalloc);
          card.list();
        }
      }
      return true;
    }

private:
    SCARDCONTEXT _context;
    bool _autoalloc = false;
};

int main()
{
    try {
      connection c(false);
      if (c.valid()) {
        c.for_each();
      }

        connection d(true);
        if (d.valid()) {
          d.for_each();
        }
    }
    catch(LONG rc) {
      std::cerr << "failed with " << err2str(rc) << std::endl;
    } catch(...) {
        std::cerr << "Unknown exception" << std::endl;
    }

    return 0;
}
