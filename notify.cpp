/**
*************************************************************
* @file notify.cpp
* @brief Breve descripción
* Pequeña documentación del archivo
*
*
*
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 14 dic 2015
* Historial de cambios:
*
*
*
*
*
*
*
*************************************************************/

#include "notify.hpp"
#include <iostream>
#include "lib/timeutils.hpp"

Notify::Notify(std :: string id, std :: string type):type(type)
{
  /* Mirar si el ID es auto y generar uno si es así */
  this->id = id;
}

Notify::~Notify()
{
}

void Notify::setOptions(std :: map < std :: string, std :: string > options)
{
  for (auto i : options)
    {
      this->option[i.first] = i.second;
    }
}

Notify::Notify(Notify && n) noexcept : id(std::move(n.id)), type(std::move(n.type)), option(std::move(n.option)) 
{
}

Notify::Notify(const Notify & n)
{
  this->id = n.id;
  this->type = n.type;
  this->option = n.option;
}
