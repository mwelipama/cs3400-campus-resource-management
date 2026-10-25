#include "Resource.h"
#include <iostream>
#include <iomanip>
#include "Utils.h"

string statusToString(AvailabilityStatus status) {
    switch (status) {
        case AvailabilityStatus::AVAILABLE:   return "Available";
        case AvailabilityStatus::BORROWED:    return "Borrowed";
        case AvailabilityStatus::MAINTENANCE: return "Maintenance";
    }
    return "Unknown";
}

AvailabilityStatus statusFromString(const string& text) {
    if (text == "Borrowed") return AvailabilityStatus::BORROWED;
    if (text == "Maintenance") return AvailabilityStatus::MAINTENANCE;
    return AvailabilityStatus::AVAILABLE;
}

LibraryResource::LibraryResource(const string& id, const string& name, const string& locationText,
                int capacityValue, AvailabilityStatus statusValue)
    : resourceId(id), resourceName(name), location(locationText),
      capacity(capacityValue), status(statusValue) {}

void LibraryResource::assignFrom(const LibraryResource& other) {
    resourceName = other.resourceName;
    location = other.location;
    capacity = other.capacity;
    status = other.status;
    setExtraValue(other.getExtraValue());
}

string LibraryResource::serialize() const {
    return getTypeCode() + "|" + resourceId + "|" + resourceName + "|" + location + "|" +
           to_string(capacity) + "|" + statusToString(status) + "|" + getExtraValue();
}

void LibraryResource::printTableHeader() {
    cout << left << setw(8) << "ID" << setw(30) << "Name" << setw(11) << "Type"
              << setw(26) << "Location" << setw(6) << "Cap" << setw(13) << "Status"
              << "Details\n";
    Utils::printLine('-', 110);
}

void LibraryResource::printRow() const {
    cout << left << setw(8) << resourceId << setw(30) << Utils::truncate(resourceName, 28)
              << setw(11) << getTypeName() << setw(26) << Utils::truncate(location, 24)
              << setw(6) << capacity << setw(13) << statusToString(status)
              << getExtraLabel() << ": " << getExtraValue() << "\n";
}

Book::Book(const string& id, const string& name, const string& locationText, int copies,
     AvailabilityStatus statusValue, const string& authorName)
    : LibraryResource(id, name, locationText, copies, statusValue), author(authorName) {}

StudyRoom::StudyRoom(const string& id, const string& name, const string& locationText, int seats,
          AvailabilityStatus statusValue, int floor)
    : LibraryResource(id, name, locationText, seats, statusValue), floorNumber(floor) {}

void             StudyRoom::setExtraValue(const string& value) {
    int parsedFloor = 0;
    floorNumber = Utils::tryParseInt(value, 0, 100, parsedFloor) ? parsedFloor : 0;
}

Equipment::Equipment(const string& id, const string& name, const string& locationText, int units,
          AvailabilityStatus statusValue, const string& condition)
    : LibraryResource(id, name, locationText, units, statusValue), conditionNote(condition) {}

LibraryResource* ResourceFactory::create(const string& typeCode, const string& id, const string& name,
                               const string& location, int capacity, AvailabilityStatus status,
                               const string& extraValue) {
    if (typeCode == "BOOK") return new Book(id, name, location, capacity, status, extraValue);
    if (typeCode == "ROOM") {
        StudyRoom* room = new StudyRoom(id, name, location, capacity, status, 0);
        room->setExtraValue(extraValue);
        return room;
    }
    if (typeCode == "EQUIPMENT") return new Equipment(id, name, location, capacity, status, extraValue);
    return nullptr;
}
