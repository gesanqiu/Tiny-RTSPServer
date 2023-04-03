//
// Created by ricardo on 4/2/23.
//

#include <sstream>
#include <iostream>
#include <algorithm>

#include "RtspMessage.h"

RTSPMessage::RTSPMessage()
        : method_(Method::UNKNOWN),
          status_code_(StatusCode::Unset),
          request_uri_(""),
          protocol_("RTSP/1.0"),
          cseq_(0) {
}

RTSPMessage::Method RTSPMessage::parse_method(const std::string& method_str) {
    static const std::map<std::string, Method> method_map = {
            {"OPTIONS", Method::OPTIONS},
            {"DESCRIBE", Method::DESCRIBE},
            {"SETUP", Method::SETUP},
            {"PLAY", Method::PLAY},
            {"PAUSE", Method::PAUSE},
            {"TEARDOWN", Method::TEARDOWN},
            {"GET_PARAMETER", Method::GET_PARAMETER},
            {"SET_PARAMETER", Method::SET_PARAMETER},
            {"REDIRECT", Method::REDIRECT},
            {"ANNOUNCE", Method::ANNOUNCE},
            {"RECORD", Method::RECORD}
    };

    auto it = method_map.find(method_str);
    if (it != method_map.end()) {
        return it->second;
    } else {
        return Method::UNKNOWN;
    }
}


std::string RTSPMessage::status_code_to_string(StatusCode status_code) const {
    switch (status_code) {
        case StatusCode::Continue: return "Continue";
        case StatusCode::OK: return "OK";
        case StatusCode::Created: return "Created";
            // ... Add other status codes here ...
        default: return "Unknown";
    }
}

std::string RTSPMessage::method_to_string(Method method) const {
    switch (method) {
        case Method::OPTIONS: return "OPTIONS";
        case Method::DESCRIBE: return "DESCRIBE";
        case Method::SETUP: return "SETUP";
        case Method::PLAY: return "PLAY";
        case Method::PAUSE: return "PAUSE";
        case Method::TEARDOWN: return "TEARDOWN";
            // ... Add other methods here ...
        default: return "UNKNOWN";
    }
}

// Implement the parse method to parse the incoming RTSP request
bool RTSPMessage::parse(const std::string& data) {
    std::istringstream iss(data);
    std::string line;

    // Read the request line
    if (!std::getline(iss, line)) {
        return false;
    }

    // Parse the method and URI
    std::istringstream request_line(line);
    std::string method_str;
    request_line >> method_str >> request_uri_ >> protocol_;
    method_ = parse_method(method_str);

    // Read and parse header lines
    while (std::getline(iss, line)) {
        // Remove carriage return character if present
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        // End of headers
        if (line.empty()) {
            break;
        }

        std::istringstream header_line(line);
        std::string header_name, header_value;
        std::getline(header_line, header_name, ':');
        std::getline(header_line, header_value);

        // Remove leading and trailing whitespaces from the header value
        auto begin_pos = header_value.find_first_not_of(' ');
        auto end_pos = header_value.find_last_not_of(' ');
        header_value = header_value.substr(begin_pos, end_pos - begin_pos + 1);

        // Check if the header is CSeq
        if (header_name == "CSeq") {
            cseq_ = std::stoi(header_value);
        } else {
            headers_[header_name] = header_value;
        }
    }

    return true;
}

std::string RTSPMessage::create_response() const {
    std::ostringstream oss;

    // Write the status line
    if (status_code_ != StatusCode::Unset) {
        oss << protocol_ << " " << static_cast<int>(status_code_) << " " << status_code_to_string(status_code_) << "\r\n";
    } else {
        oss << method_to_string(method_) << " " << request_uri_ << " " << protocol_ << "\r\n";
    }

    // Write headers
    for (const auto& header : headers_) {
        oss << header.first << ": " << header.second << "\r\n";
    }

    // Write an empty line to separate headers from the body
    oss << "\r\n";

    // Write the body, if any
    if (!body_.empty()) {
        oss << body_;
    }

    return oss.str();
}

const std::string& RTSPMessage::uri() const {
    return request_uri_;
}

void RTSPMessage::set_cseq(unsigned int cseq) {
    cseq_ = cseq;
}

unsigned int RTSPMessage::cseq() const {
    return cseq_;
}

void RTSPMessage::set_status_code(StatusCode status_code) {
    status_code_ = status_code;
}

RTSPMessage::StatusCode RTSPMessage::status_code() const {
    return status_code_;
}

void RTSPMessage::set_method(Method method) {
    method_ = method;
}

RTSPMessage::Method RTSPMessage::method() const {
    return method_;
}

const std::string& RTSPMessage::protocol() const {
    return protocol_;
}

void RTSPMessage::set_protocol(const std::string& protocol) {
    protocol_ = protocol;
}

void RTSPMessage::set_headers(std::string key, std::string value) {
    headers_[key] = value;
}

std::string RTSPMessage::get_header_value(const std::string key) const {
    if (auto it  = headers_.find(key); it != headers_.end()) return it->second;
    return {};
}

void RTSPMessage::set_body(const std::string &body) {
    body_ = body;
}
