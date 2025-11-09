// debug.h - extended logger + optional simple profiler
#pragma once
#include <cstdio>
#include <chrono>
#include <unordered_map>
#include <string>

#ifndef ST_DEBUG
#define ST_DEBUG 1
#endif

// --- Levelled logger ---
namespace _st_internal { inline auto LogStart() { return std::chrono::steady_clock::now(); } inline const auto kLogStart = LogStart(); inline long long MsSinceStart(){ using namespace std::chrono; return duration_cast<milliseconds>(steady_clock::now()-kLogStart).count(); } }

#if ST_DEBUG
	#define LOG_DEBUG(fmt, ...) std::printf("[%6lldms][DEBUG] %s:%d: " fmt "\n", _st_internal::MsSinceStart(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
	#define LOG_DEBUG(fmt, ...) do{}while(0)
#endif

#define LOG_INFO(fmt, ...)  std::printf("[%6lldms][INFO ] " fmt "\n", _st_internal::MsSinceStart(), ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  std::printf("[%6lldms][WARN ] " fmt "\n", _st_internal::MsSinceStart(), ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) std::fprintf(stderr,"[%6lldms][ERROR] %s:%d: " fmt "\n", _st_internal::MsSinceStart(), __FILE__, __LINE__, ##__VA_ARGS__)

// Backwards compatibility for old macro
#define DEBUG_LOG LOG_DEBUG

// LOG_ONCE - wypisz komunikat tylko raz (klucz string literal)
#if ST_DEBUG
	inline bool _log_once_hit(const char* key){
		static std::unordered_map<std::string,bool> flags; auto it=flags.find(key); if(it!=flags.end()) return false; flags[key]=true; return true; }
	#define LOG_ONCE(key, fmt, ...) do { if(_log_once_hit(key)) { LOG_INFO("(once:%s) " fmt, key, ##__VA_ARGS__); } } while(0)
#else
	#define LOG_ONCE(key, fmt, ...) do{}while(0)
#endif

// --- Profiler RAII (opcjonalny) ---
#ifndef ST_PROFILE
#define ST_PROFILE 0
#endif

#if ST_DEBUG && ST_PROFILE
class ProfScope {
public:
	ProfScope(const char* name): _name(name), _start(std::chrono::high_resolution_clock::now()){}
	~ProfScope(){
		using namespace std::chrono;
		auto end=high_resolution_clock::now();
		auto us = duration_cast<microseconds>(end-_start).count();
		std::printf("[PROF ] %s: %lld us\n", _name, (long long)us);
	}
private:
	const char* _name; std::chrono::high_resolution_clock::time_point _start;
};
	#define PROF_SCOPE(name) ProfScope _prof_##__LINE__{name}
#else
	#define PROF_SCOPE(name) do{}while(0)
#endif

