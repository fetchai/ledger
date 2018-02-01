#ifndef HTTP_STATUS_HPP
#define HTTP_STATUS_HPP
namespace fetch
{
namespace http 
{

struct Status 
{
  uint16_t code;
  std::string explanation;  
};

namespace status_code {
Status const UNKNOWN = {0, "100 Continue"};
Status const INFORMATION_CONTINUE = {100,  "101 Switching Protocols"};
Status const INFORMATION_SWITCHING_PROTOCOLS = {101, "102 Processing"};
Status const INFORMATION_PROCESSING = {102, ""};
Status const SUCCESS_OK = {200, "200 OK"};
Status const SUCCESS_CREATED = {201, "201 Created"};
Status const SUCCESS_ACCEPTED = {202, "202 Accepted"};
Status const SUCCESS_NON_AUTHORITATIVE_INFORMATION = {203, "203 Non-Authoritative Information"};
Status const SUCCESS_NO_CONTENT = {204, "204 No Content"};
Status const SUCCESS_RESET_CONTENT = {205, "205 Reset Content"};
Status const SUCCESS_PARTIAL_CONTENT ={206, "206 Partial Content"};
Status const SUCCESS_MULTI_STATUS = {207, "207 Multi-Status"};
Status const SUCCESS_ALREADY_REPORTED = {208, "208 Already Reported"};
Status const SUCCESS_IM_USED = {226, "226 IM Used"};
Status const REDIRECTION_MULTIPLE_CHOICES = {300, "300 Multiple Choices"};
Status const REDIRECTION_MOVED_PERMANENTLY = {301, "301 Moved Permanently"};
Status const REDIRECTION_FOUND = {302,  "302 Found"};
Status const REDIRECTION_SEE_OTHER = {303, "303 See Other"};
Status const REDIRECTION_NOT_MODIFIED = {304, "304 Not Modified"};
Status const REDIRECTION_USE_PROXY = {305,  "305 Use Proxy"};
Status const REDIRECTION_SWITCH_PROXY = {306, "306 Switch Proxy"};
Status const REDIRECTION_TEMPORARY_REDIRECT = {307, "307 Temporary Redirect"};
Status const REDIRECTION_PERMANENT_REDIRECT = {308, "308 Permanent Redirect"};
Status const CLIENT_ERROR_BAD_REQUEST = {400, "400 Bad Request"};
Status const CLIENT_ERROR_UNAUTHORIZED = {401, "401 Unauthorized"};
Status const CLIENT_ERROR_PAYMENT_REQUIRED = {402, "402 Payment Required"};
Status const CLIENT_ERROR_FORBIDDEN = {403, "403 Forbidden"};
Status const CLIENT_ERROR_NOT_FOUND = {404, "404 Not Found"};
Status const CLIENT_ERROR_METHOD_NOT_ALLOWED = {405, "405 Method Not Allowed"};
Status const CLIENT_ERROR_NOT_ACCEPTABLE = {406, "406 Not Acceptable"};
Status const CLIENT_ERROR_PROXY_AUTHENTICATION_REQUIRED = {407, "407 Proxy Authentication Required"};
Status const CLIENT_ERROR_REQUEST_TIMEOUT = {408, "408 Request Timeout"};
Status const CLIENT_ERROR_CONFLICT = {409, "409 Conflict"};
Status const CLIENT_ERROR_LENGTH_REQUIRED = {411, "411 Length Required"};
Status const CLIENT_ERROR_PRECONDITION_FAILED = {412, "412 Precondition Failed"};
Status const CLIENT_ERROR_PAYLOAD_TOO_LARGE = {413, "413 Payload Too Large"};
Status const CLIENT_ERROR_URI_TOO_LONG = {414, "414 URI Too Long"};
Status const CLIENT_ERROR_UNSUPPORTED_MEDIA_TYPE = {415, "415 Unsupported Media Type"};
Status const CLIENT_ERROR_RANGE_NOT_SATISFIABLE = {416, "416 Range Not Satisfiable"};
Status const CLIENT_ERROR_EXPECTATION_FAILED = {417, "417 Expectation Failed"};
Status const CLIENT_ERROR_IM_A_TEAPOT = {418, "418 I'm a teapot"};
Status const CLIENT_ERROR_MISDIRECTION_REQUIRED = {421, "421 Misdirected Request"};
Status const CLIENT_ERROR_UNPROCESSABLE_ENTITY = {422, "422 Unprocessable Entity"};
Status const CLIENT_ERROR_LOCKED = {423, "423 Locked"};
Status const CLIENT_ERROR_FAILED_DEPENDENCY = {424, "424 Failed Dependency"};
Status const CLIENT_ERROR_UPGRADE_REQUIRED = {426, "426 Upgrade Required"};
Status const CLIENT_ERROR_PRECONDITION_REQUIRED = {428, "428 Precondition Required"};
Status const CLIENT_ERROR_TOO_MANY_REQUESTS = {429, "429 Too Many Requests"};
Status const CLIENT_ERROR_REQUEST_HEADER_FIELDS_TOO_LARGE = {431, "431 Request Header Fields Too Large"};
Status const CLIENT_ERROR_UNAVAILABLE_FOR_LEGAL_REASONS = {451, "451 Unavailable For Legal Reasons"};
Status const SERVER_ERROR_INTERNAL_SERVER_ERROR = {500, "500 Internal Server Error"};
Status const SERVER_ERROR_NOT_IMPLEMENTED = {501,  "501 Not Implemented"};
Status const SERVER_ERROR_BAD_GATEWAY = {502, "502 Bad Gateway"};
Status const SERVER_ERROR_SERVICE_UNAVAILABLE = {503, "503 Service Unavailable"};
Status const SERVER_ERROR_GATEWAY_TIMEOUT = {504, "504 Gateway Timeout"};
Status const SERVER_ERROR_HTTP_VERSION_NOT_SUPPORTED = {505, "505 HTTP Version Not Supported"};
Status const SERVER_ERROR_VARIANT_ALSO_NEGOTIATES = {506, "506 Variant Also Negotiates"};
Status const SERVER_ERROR_INSUFFICIENT_STORAGE = {507, "507 Insufficient Storage"};
Status const SERVER_ERROR_LOOP_DETECTED = {508, "508 Loop Detected"};
Status const SERVER_ERROR_NOT_EXTENDED = {510, "510 Not Extended"};
Status const SERVER_ERROR_NETWORK_AUTHENTICATION_REQUIRED = {511, "511 Network Authentication Required"};
  
};
 
};
};

#endif
