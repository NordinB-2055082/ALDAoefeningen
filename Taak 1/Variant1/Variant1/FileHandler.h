// AUTEURS: Nordin Ben-AL-Lal, Abdullah Yalvac
#pragma once

#include "AddressBook.h"

class FileHandler {
public:
    static void readContactsFromFile(const std::string& fileName, AddressBook& addressBook);
};