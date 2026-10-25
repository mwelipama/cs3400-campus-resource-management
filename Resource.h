#ifndef RESOURCE_H
#define RESOURCE_H

#include <string>

using namespace std;

enum class AvailabilityStatus { AVAILABLE, BORROWED, MAINTENANCE };

string statusToString(AvailabilityStatus status);

AvailabilityStatus statusFromString(const string& text);

// Abstract class: it cannot be instantiated; Book, StudyRoom and Equipment
// supply the type-specific behaviour through virtual functions.
class LibraryResource {
protected:
    string        resourceId;
    string        resourceName;
    string        location;
    int                capacity;   // Book: copies, StudyRoom: seats, Equipment: units
    AvailabilityStatus status;

public:
    LibraryResource(const string& id, const string& name, const string& locationText,
                    int capacityValue, AvailabilityStatus statusValue);

    LibraryResource(const LibraryResource&) = default;
    virtual ~LibraryResource() {}

    virtual string       getTypeName() const = 0;
    virtual string       getTypeCode() const = 0;
    virtual string       getExtraLabel() const = 0;
    virtual string       getExtraValue() const = 0;
    virtual void              setExtraValue(const string& value) = 0;
    virtual LibraryResource*  clone() const = 0;

    // Copies every editable field from another resource (used by UNDO of an update).
    virtual void assignFrom(const LibraryResource& other);

    const string& getId() const { return resourceId; }
    const string& getName() const { return resourceName; }
    const string& getLocation() const { return location; }
    int                getCapacity() const { return capacity; }
    AvailabilityStatus getStatus() const { return status; }

    void setName(const string& newName) { resourceName = newName; }
    void setLocation(const string& newLocation) { location = newLocation; }
    void setCapacity(int newCapacity) { capacity = newCapacity; }
    void setStatus(AvailabilityStatus newStatus) { status = newStatus; }

    string serialize() const;

    static void printTableHeader();

    void printRow() const;
};

class Book : public LibraryResource {
private:
    string author;

public:
    Book(const string& id, const string& name, const string& locationText, int copies,
         AvailabilityStatus statusValue, const string& authorName);

    string      getTypeName() const override { return "Book"; }
    string      getTypeCode() const override { return "BOOK"; }
    string      getExtraLabel() const override { return "Author"; }
    string      getExtraValue() const override { return author; }
    void             setExtraValue(const string& value) override { author = value; }
    LibraryResource* clone() const override { return new Book(*this); }
};

class StudyRoom : public LibraryResource {
private:
    int floorNumber;

public:
    StudyRoom(const string& id, const string& name, const string& locationText, int seats,
              AvailabilityStatus statusValue, int floor);

    string      getTypeName() const override { return "StudyRoom"; }
    string      getTypeCode() const override { return "ROOM"; }
    string      getExtraLabel() const override { return "Floor"; }
    string      getExtraValue() const override { return to_string(floorNumber); }
    void             setExtraValue(const string& value) override;
    LibraryResource* clone() const override { return new StudyRoom(*this); }
};

class Equipment : public LibraryResource {
private:
    string conditionNote;

public:
    Equipment(const string& id, const string& name, const string& locationText, int units,
              AvailabilityStatus statusValue, const string& condition);

    string      getTypeName() const override { return "Equipment"; }
    string      getTypeCode() const override { return "EQUIPMENT"; }
    string      getExtraLabel() const override { return "Condition"; }
    string      getExtraValue() const override { return conditionNote; }
    void             setExtraValue(const string& value) override { conditionNote = value; }
    LibraryResource* clone() const override { return new Equipment(*this); }
};

class ResourceFactory {
public:
    static LibraryResource* create(const string& typeCode, const string& id, const string& name,
                                   const string& location, int capacity, AvailabilityStatus status,
                                   const string& extraValue);
};
#endif
