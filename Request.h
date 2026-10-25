#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <ctime>

using namespace std;

enum class RequestStatus { PENDING, APPROVED, REJECTED, CANCELLED };
enum class PriorityLevel { EMERGENCY = 1, HIGH = 2, NORMAL = 3, LOW = 4 };

string requestStatusToString(RequestStatus status);

RequestStatus requestStatusFromString(const string& text);

string priorityToString(PriorityLevel priority);

class ResourceRequest {
private:
    int           requestId;
    string   userName;
    string   requestedResourceId;
    time_t   submissionTime;
    RequestStatus status;
    PriorityLevel priority;

public:
    ResourceRequest();

    ResourceRequest(int id, const string& user, const string& resourceId, time_t time,
                    RequestStatus statusValue, PriorityLevel priorityValue);

    int                getRequestId() const { return requestId; }
    const string& getUserName() const { return userName; }
    const string& getRequestedResourceId() const { return requestedResourceId; }
    time_t        getSubmissionTime() const { return submissionTime; }
    RequestStatus      getStatus() const { return status; }
    PriorityLevel      getPriority() const { return priority; }
    void               setStatus(RequestStatus newStatus) { status = newStatus; }

    // "a < b" means a is MORE urgent than b: lower priority number first, and for equal
    // priority the older request (smaller ID) first, so equal priorities stay FIFO.
    bool operator<(const ResourceRequest& other) const;

    string formattedTime() const;

    string serialize() const;

    static void printTableHeader();

    void printRow() const;
};
#endif
