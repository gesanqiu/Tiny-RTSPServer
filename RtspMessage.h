//
// Created by ricardo on 4/2/23.
//

#ifndef RTSP_MESSAGE_H
#define RTSP_MESSAGE_H

#include <string>
#include <map>

class RTSPMessage {
public:
    enum class Method {
        OPTIONS,
        DESCRIBE,
        SETUP,
        PLAY,
        PAUSE,
        TEARDOWN,
        GET_PARAMETER,
        SET_PARAMETER,
        REDIRECT,
        ANNOUNCE,
        RECORD,
        UNKNOWN
    };
    enum class StatusCode {
        Unset = 0,
        Continue = 100,
        OK = 200,
        Created = 201,
        LowOnStorageSpace = 250,
        MultipleChoices = 300,
        MovedPermanently = 301,
        MovedTemporarily = 302,
        BadRequest = 400,
        Unauthorized = 401,
        PaymentRequired = 402,
        Forbidden = 403,
        NotFound = 404,
        MethodNotAllowed = 405,
        NotAcceptable = 406,
        ProxyAuthenticationRequired = 407,
        RequestTimeout = 408,
        Conflict = 409,
        Gone = 410,
        LengthRequired = 411,
        PreconditionFailed = 412,
        RequestEntityTooLarge = 413,
        RequestURITooLarge = 414,
        UnsupportedMediaType = 415,
        InvalidParameter = 451,
        IllegalConferenceIdentifier = 452,
        NotEnoughBandwidth = 453,
        SessionNotFound = 454,
        MethodNotValidInThisState = 455,
        HeaderFieldNotValidForResource = 456,
        InvalidRange = 457,
        ParameterIsReadOnly = 458,
        AggregateOperationNotAllowed = 459,
        OnlyAggregateOperationAllowed = 460,
        UnsupportedTransport = 461,
        DestinationUnreachable = 462,
        InternalServerError = 500,
        NotImplemented = 501,
        BadGateway = 502,
        ServiceUnavailable = 503,
        GatewayTimeout = 504,
        RTSPVersionNotSupported = 505,
        OptionNotSupported = 551
    };

    RTSPMessage();

    bool parse(const std::string& data);
    std::string create_response() const;

    Method parse_method(const std::string& method_str);
    std::string method_to_string(Method method) const;
    void set_method(Method method);
    Method method() const;

    void set_cseq(unsigned int cseq);
    unsigned int cseq() const;

    std::string status_code_to_string(StatusCode status_code) const;
    void set_status_code(StatusCode status_code);
    StatusCode status_code() const;

    const std::string& protocol() const;
    void set_protocol(const std::string& protocol);

    void set_headers(std::string key, std::string value);
    std::string get_header_value(const std::string key) const;

    const std::string& uri() const;
    const std::string& session() const;

    void set_body(const std::string& body);

private:
    std::string protocol_;
    Method method_;
    std::string request_uri_;
    unsigned int cseq_;
    std::string session_;
    StatusCode status_code_;
    std::string body_;
    std::map<std::string, std::string> headers_;
};

#endif // RTSP_MESSAGE_H
