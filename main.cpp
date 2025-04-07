#include <iostream>
#include <map>
#include <string.h>
#include <winscard.h>
#include <reader.h>

using namespace std;

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

class cardhandle {
public:
    cardhandle(SCARDCONTEXT context, const std::string& name) :_name(name) {
        DWORD active = 0;
        auto rc = SCardConnect(context, name.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_ANY, &_card, &active);
        std::cout << "SCardConnect: " << name << ": " << pcsc_stringify_error(rc) << std::endl;
        if (rc != SCARD_S_SUCCESS)
            throw rc;
    }
    ~cardhandle() {
        auto rc = SCardDisconnect(_card, 0);
        std::cout << "SCardDisconnect: " << pcsc_stringify_error(rc) << std::endl;
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
            auto rc = SCardGetAttrib(_card, attr.first, nullptr, &len);
            std::cout << "SCardGetAttrib: (1): " << attr.second << ": " << pcsc_stringify_error(rc) << std::endl;
            if (rc != SCARD_S_SUCCESS)
                continue;

            std::string sattr;
            sattr.resize(len);

            auto rc2 = SCardGetAttrib(_card, attr.first, reinterpret_cast<LPBYTE>(sattr.data()), &len);
            std::cout << "SCardGetAttrib: (2): " << rc2 << std::endl;
            if (rc2 != SCARD_S_SUCCESS)
                throw rc2;
            std::cout << "SCardGetAttrib: (2): " << attr.second << ": [" << len << "] {"<<string_to_hex(sattr) << "} '" << sattr.c_str() << "'" << std::endl;
        }
        return true;
    }

    private:
        SCARDHANDLE _card;
        std::string _name;
};

class connection {
public:
    connection() {
        auto rc = SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr, nullptr, &_context);
        std::cout << "SCardEstablishContext: " << pcsc_stringify_error(rc) << std::endl;
        if (rc != SCARD_S_SUCCESS)
            throw rc;
    }

    ~connection() {
        auto rc = SCardReleaseContext(_context);
        std::cout << "SCardReleaseContext: " << pcsc_stringify_error(rc) << std::endl;
    }

    bool valid() const {
        auto rc = SCardIsValidContext(_context);
        return rc == SCARD_S_SUCCESS;
    }

    bool foreach() {
        DWORD count = 0;
        auto rc = SCardListReaders(_context, nullptr, nullptr, &count);
        std::cout << "SCardListReaders: " << pcsc_stringify_error(rc) << std::endl;
        if (rc != SCARD_S_SUCCESS)
            throw rc;

        std::string list;
        list.resize(count);
        auto rc2 = SCardListReaders(_context, nullptr, list.data(), &count);
        std::cout << "SCardListReaders: " << pcsc_stringify_error(rc) << std::endl;
        if (rc2 != SCARD_S_SUCCESS)
            throw rc2;

        size_t pos = 0;
        while (pos < count) {
            const char* str = &list.c_str()[pos];
            const size_t len = strnlen(str, count);
            pos += len + 1;
            if (len > 0) {
                std::cout << "reader: " << str << std::endl;
                cardhandle card(_context, str);
                card.list();
            }
        }
        return true;
    }

private:
    SCARDCONTEXT _context;
};

int main()
{
    try {
        connection c;
        if (c.valid()) {
            c.foreach();
        }
    }
    catch(LONG rc) {
        std::cerr << "failed with " << pcsc_stringify_error(rc) << std::endl;
    } catch(...) {
        std::cerr << "Unknown exception" << std::endl;
    }

    return 0;
}
