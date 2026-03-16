#ifndef __VOCAB_H__
#define __VOCAB_H__

#include <string>

std::string load_clip_merges();
std::string load_qwen2_merges();
std::string load_mistral_merges();
std::string load_mistral_vocab_json();
std::string load_t5_tokenizer_json();
std::string load_umt5_tokenizer_json();

// Set the directory from which external vocab files are loaded at runtime.
// Call before any load_* function. If not called, load_* returns empty string
// (or falls back to embedded arrays when WAN_EMBED_VOCAB is defined).
void wan_vocab_set_dir(const std::string& dir);

#endif  // __VOCAB_H__
