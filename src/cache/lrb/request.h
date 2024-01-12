// Siyuan: hack for key-value caching
#ifndef WEBCACHESIM_REQUEST_H
#define WEBCACHESIM_REQUEST_H

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

//using namespace std;

namespace covered
{
    // Request information
    class SimpleRequest {
    public:
        SimpleRequest() = default;

        virtual ~SimpleRequest() = default;

        SimpleRequest(
                const int64_t &_id,
                const int64_t &_size,
                const bool& is_keybased_req, const std::string& keystr, const std::string& valuestr) {
            reinit(0, _id, _size, is_keybased_req, keystr, valuestr);
        }

        SimpleRequest(const int64_t &_seq, const int64_t &_id, const int64_t &_size, const bool& is_keybased_req, const std::string& keystr, const std::string& valuestr, const std::vector<uint16_t> *_extra_features = nullptr) {
            reinit(_seq, _id, _size, is_keybased_req, keystr, valuestr, _extra_features);
        }

        void reinit(const int64_t &_seq, const int64_t &_id, const int64_t &_size, const bool& is_keybased_req, const std::string& keystr, const std::string& valuestr, const std::vector<uint16_t> *_extra_features = nullptr) {
            seq = _seq;
            id = _id;
            size = _size;
            if (_extra_features) {
                extra_features = *_extra_features;
            }

            // Siyuan: for key-value caching
            this->is_keybased_req = is_keybased_req;
            this->keystr = keystr;
            this->valuestr = valuestr;
        }

        uint64_t seq{};
        int64_t id{}; // request object id
        int64_t size{}; // request size in bytes
        //category feature. unsigned int. ideally not exceed 2k
        std::vector<uint16_t > extra_features;

        // Siyuan: for key-value caching
        bool is_keybased_req = false;
        std::string keystr;
        std::string valuestr;
    };


    class AnnotatedRequest : public SimpleRequest {
    public:
        AnnotatedRequest() = default;

        // Create request
        AnnotatedRequest(const int64_t &_seq, const int64_t &_id, const int64_t &_size, const int64_t &_next_seq,
                        const bool& is_keybased_req, const std::string& keystr, const std::string& valuestr,
                        const std::vector<uint16_t> *_extra_features = nullptr)
                : SimpleRequest(_seq, _id, _size, is_keybased_req, keystr, valuestr, _extra_features),
                next_seq(_next_seq) {}

        void reinit(const int64_t &_seq, const int64_t &_id, const int64_t &_size, const int64_t &_next_seq,
                        const bool& is_keybased_req, const std::string& keystr, const std::string& valuestr,
                        const std::vector<uint16_t> *_extra_features = nullptr) {
            SimpleRequest::reinit(_seq, _id, _size, is_keybased_req, keystr, valuestr, _extra_features);
            next_seq = _next_seq;
        }

        int64_t next_seq{};
    };
}

#endif /* REQUEST_H */



