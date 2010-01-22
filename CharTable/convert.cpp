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
  std::cout << "CHARACTER_TABLE CODE\n"
	    << "char_table\n"
	    << "\tglobal char_table\n";

  std::cout << "\tdb ." << 4
	    << ",LOW(char_data_" << 0  
	    << "),HIGH(char_data_" << 0
	    << "),UPPER(char_data_" << 0
	    << ")\n";


  for (size_t i(1); i < nr_chrs_S; ++i)
    std::cout << "\tdb ." << int(lentbl_S[i])
	      << ",LOW(char_data_" << i  
	      << "),HIGH(char_data_" << i
	      << "),UPPER(char_data_" << i
	      << ")\n";

  std::cout << "\n";

  std::cout << "char_data_" << 0
	    << "\n\tdb 0x00,0x00,0x00,0x00\n";


  for (size_t i(1); i < nr_chrs_S; ++i)
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
  
      std::cout << " end\n";

  return 0;
}
