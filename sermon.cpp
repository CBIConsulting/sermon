/**
*************************************************************
* @file sermon.cpp
* @brief Server Monitor
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 09 dic 2015
* Historial de cambios:
*
* Ya se irá mejorando el código. Por ahora, para salir del paso podemos
* dejarlo así
*
*************************************************************/

#include <iostream>
#include <memory>
#include "sermon_exception.hpp"
#include "sermon_app.hpp"
#include "lib/tcolor.hpp"
#include <thread>


int main(int argc, char *argv[])
{
  std::shared_ptr<Sermon> s;

  try
    {
      s = std::make_shared<Sermon>();
    }
  catch (SermonException &e)
    {
      std::cout << TColor(TColor::RED) << "Error: "<<e.what() << endColor << std::endl;
      return 1;
    }

	s-> monitoring();

	return EXIT_SUCCESS;
}

