#include "Request.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include "Utils.h"

string requestStatusToString(RequestStatus status) {
    switch (status) {
        case RequestStatus::PENDING:   return "Pending";
        case RequestStatus::APPROVED:  return "Approved";
        case RequestStatus::REJECTED:  return "Rejected";
        case RequestStatus::CANCELLED: return "Cancelled";
    }
    return "Unknown";
}

RequestStatus requestStatusFromString(const string& text) {
    if (text == "Approved") return RequestStatus::APPROVED;
    if (text == "Rejected") return RequestStatus::REJECTED;
    if (text == "Cancelled") return RequestStatus::CANCELLED;
    return RequestStatus::PENDING;
}

string priorityToString(PriorityLevel priority) {
    switch (priority) {
        case PriorityLevel::EMERGENCY: return "Emergency";
        case PriorityLevel::HIGH:      return "High";
        case PriorityLevel::NORMAL:    return "Normal";
        case PriorityLevel::LOW:       return "Low";
    }
    return "Unknown";
}

ResourceRequest::ResourceRequest()
    : requestId(0), submissionTime(0), status(RequestStatus::PENDING), priority(PriorityLevel::NORMAL) {}

ResourceRequest::ResourceRequest(int id, const string& user, const string& resourceId, time_t time,
                RequestStatus statusValue, PriorityLevel priorityValue)
    : requestId(id), userName(user), requestedResourceId(resourceId), submissionTime(time),
      status(statusValue), priority(priorityValue) {}

bool ResourceRequest::operator<(const ResourceRequest& other) const {
    if (priority != other.priority) return static_cast<int>(priority) < static_cast<int>(other.priority);
    return requestId < other.requestId;
}

string ResourceRequest::formattedTime() const {
    char buffer[32];
    tm* localTime = localtime(&submissionTime);
    if (localTime == nullptr) return "unknown";
    strftime(buffer, sizeof(buffer), "%d-%m %H:%M:%S", localTime);
    return buffer;
}

string ResourceRequest::serialize() const {
    return to_string(requestId) + "|" + userName + "|" + requestedResourceId + "|" +
           to_string(static_cast<long long>(submissionTime)) + "|" + requestStatusToString(status) + "|" +
           to_string(static_cast<int>(priority));
}

void ResourceRequest::printTableHeader() {
    cout << left << setw(6) << "ReqID" << setw(20) << "User" << setw(10) << "Resource"
              << setw(11) << "Priority" << setw(16) << "Submitted" << "Status\n";
    Utils::printLine('-', 75);
}

void ResourceRequest::printRow() const {
    cout << left << setw(6) << requestId << setw(20) << Utils::truncate(userName, 18)
              << setw(10) << requestedResourceId << setw(11) << priorityToString(priority)
              << setw(16) << formattedTime() << requestStatusToString(status) << "\n";
}
