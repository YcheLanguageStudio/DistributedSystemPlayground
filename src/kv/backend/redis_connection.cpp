#include "redis_connection.h"

#include <cstring>

namespace yche::kv::internal {

RedisConnection::RedisConnection() = default;

RedisConnection::~RedisConnection() {
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

bool RedisConnection::connect(const std::string& host, int port) {
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
    timeval tv{};
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    ctx_ = redisConnectWithTimeout(host.c_str(), port, tv);
    if (!ctx_ || ctx_->err) {
        if (ctx_) {
            redisFree(ctx_);
            ctx_ = nullptr;
        }
        return false;
    }
    return true;
}

bool RedisConnection::get(const std::string& key, std::string& value, KvError* err) {
    if (!ctx_) {
        *err = KvError::BACKEND_ERROR;
        return false;
    }
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "GET %b", key.data(), key.size()));
    if (!reply) {
        *err = KvError::BACKEND_ERROR;
        return false;
    }
    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        *err = KvError::KEY_NOT_FOUND;
        return false;
    }
    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        *err = KvError::BACKEND_ERROR;
        return false;
    }
    value.assign(reply->str, reply->len);
    freeReplyObject(reply);
    *err = KvError::OK;
    return true;
}

bool RedisConnection::set(const std::string& key, const std::string& val, int expire_ms, KvError* err) {
    if (!ctx_) {
        *err = KvError::BACKEND_ERROR;
        return false;
    }
    redisReply* reply = nullptr;
    if (expire_ms > 0) {
        reply = static_cast<redisReply*>(
            redisCommand(ctx_, "SET %b %b PX %d", key.data(), key.size(), val.data(), val.size(), expire_ms));
    } else {
        reply = static_cast<redisReply*>(
            redisCommand(ctx_, "SET %b %b", key.data(), key.size(), val.data(), val.size()));
    }
    if (!reply) {
        *err = KvError::BACKEND_ERROR;
        return false;
    }
    const bool ok = (reply->type == REDIS_REPLY_STATUS && reply->str && std::strcmp(reply->str, "OK") == 0);
    freeReplyObject(reply);
    if (!ok) {
        *err = KvError::BACKEND_ERROR;
        return false;
    }
    *err = KvError::OK;
    return true;
}

}  // namespace yche::kv::internal
