namespace utils {
using std::string;
using namespace eosio;

string to_hex(const char *d, uint32_t s) {
    std::string r;
    const char *to_hex = "0123456789abcdef";
    uint8_t *c = (uint8_t *)d;
    for (uint32_t i = 0; i < s; ++i)
        (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
    return r;
}

string sha256_to_hex(const checksum256 &sha256) {
    auto hash_data = sha256.extract_as_byte_array();
    return to_hex((char *)hash_data.data(), sizeof(hash_data.data()));
}

uint64_t uint64_hash(const string &hash) { return std::hash<string>{}(hash); }

uint64_t uint64_hash(const checksum256 &hash) {
    return uint64_hash(sha256_to_hex(hash));
}

void split(const std::string &str, char delimiter, std::vector<std::string> &params) {
    std::size_t cur, prev = 0;
    cur = str.find(delimiter);
    while (cur != std::string::npos) {
        params.push_back(str.substr(prev, cur - prev));
        prev = cur + 1;
        cur = str.find(delimiter, prev);
    }
    params.push_back(str.substr(prev));
}
}