#include <iostream>
#include "Font8.h"
#include <boost/foreach.hpp>

void printByte(const BYTE bitfield)
{
    BYTE mask = 0X01;
    
    std::cout << "b'";
    while( mask )
    {
        if( (mask & bitfield) )
	  std::cout << '1';
        else
	  std::cout << '0';

        mask = mask << 1 ;
    }
    std::cout << "'";
}

int main()
{
  std::cout << "char_table\n";

  for (size_t i(0); i < nr_chrs_S; ++i)
    std::cout << "\tdb ." << (int(lentbl_S[i]) ? int(lentbl_S[i]): 1)
	      << ",LOW(char_data_" << i  
	      << "),HIGH(char_data_" << i
	      << "),UPPER(char_data_" << i
	      << ")\n";

  std::cout << "\n";

  for (size_t i(0); i < nr_chrs_S; ++i)
    {
      std::cout << "char_data_" << i
		<< "\n\tdb ";
      printByte(chrtbl_S[i][0]);
      
      for (size_t j(1); j < lentbl_S[i]; ++j)
	{
	  std::cout << ","; 
	  printByte(chrtbl_S[i][j]);
	}
      std::cout << "\n";
    }

  return 0;
}
