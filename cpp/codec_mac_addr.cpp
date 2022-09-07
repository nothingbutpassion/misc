#include <string>
#include <iostream>

// raw mac address style: "2C:33:61:5E:4D:FA"

std::string encodeMacAddress(const std::string& raw)
{
   // Checking
   for (const auto& c: raw)
   {
      if (! (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || (c == ':')) )
      {
         return raw;
      }
   }

   // Converting
   std::string result = raw;
   for (auto& c: result)
   {
      c += 20;
   }
   std::cout << "encodeMacAddress" << result << std::endl;
   return result;
}


std::string decodeMacAddress(const std::string& encoded)
{
   // Converting
   std::string result = encoded;
   for (auto& c: result)
   {
      c -= 20;
   }

   // Checking
   for (const auto& c: result)
   {
      if (! (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || (c == ':')) )
      {
         return encoded;
      }
   }
   return result;
}



int main()
{
   // std::string raw = "2C:33:61:5E:4D:FA";
   std::string raw = "AB:33:61:5E:4D:FA";
   std::string encoded = encodeMacAddress(raw);
   raw = decodeMacAddress(encoded);
   std::cout << "raw: " << raw << std::endl;
   std::cout << "encoded: " << encoded << std::endl;
   std::cout << "decoded: " << raw << std::endl;
   return 0;
}

