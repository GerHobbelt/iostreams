// (C) Copyright Jonathan Turkanis 2003.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_DICTIONARY_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_DICTIONARY_FILTER_HPP_INCLUDED

#include <cassert>
#include <cstdio>    // EOF.
#include <iostream>  // cin, cout.
#include <locale>
#include <map>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/filter/stdio_filter.hpp>
#include <boost/iostreams/operations.hpp>

namespace boost { namespace iostreams { namespace example {

class dictionary {
public:
    dictionary(const std::locale& loc = std::locale::classic());
    void add(std::string key, const std::string& value);
    bool replace(std::string& key);
    const std::locale& getloc() const;
    std::string::size_type max_length() const { return max_length_; }
private:
    typedef std::map<std::string, std::string> map_type;
    void tolower(std::string& str);
    map_type                map_;
    std::locale             loc_;
    std::string::size_type  max_length_;
};

class dictionary_stdio_filter : public stdio_filter {
public:
    dictionary_stdio_filter(dictionary& d) : dictionary_(d) { }
private:
    void do_filter()
    {
        using namespace std;
        while (true) {
            int c = std::cin.get();
            if (c == EOF || !std::isalpha(c, dictionary_.getloc())) {
                dictionary_.replace(current_word_);
                cout.write( current_word_.data(), 
                            static_cast<streamsize>(current_word_.size()) );
                current_word_.clear();
                if (c == EOF)
                    break;
                cout.put(c);  
            } else {
                current_word_ += c;
            }
        }
    }
    dictionary&  dictionary_;
    std::string  current_word_;
};

class dictionary_input_filter : public input_filter {
public:
    dictionary_input_filter(dictionary& d) 
        : dictionary_(d), off_(std::string::npos), eof_(false) 
        { }

    template<typename Source>
    int get(Source& src)
        {   
            // Handle unfinished business.
            if (eof_) 
                return EOF;
            if (off_ != std::string::npos && off_ < current_word_.size()) 
                return current_word_[off_++];
            if (off_ == current_word_.size()) { 
                current_word_.clear();
                off_ = std::string::npos;
            }

            // Compute curent word.
            while (true) {
                int c;
                if ((c = iostreams::get(src)) == WOULD_BLOCK)
                    return WOULD_BLOCK;

                if (c == EOF || !std::isalpha(c, dictionary_.getloc())) {
                    dictionary_.replace(current_word_);
                    off_ = 0;
                    if (c == EOF)
                        eof_ = true;
                    else
                        current_word_ += c;
                    break;
                } else {
                    current_word_ += c;
                } 
            }

            return this->get(src); // Note: current_word_ is not empty.
        }

    template<typename Source>
    void close(Source&) 
    { 
        current_word_.clear(); 
        off_ = std::string::npos;
        eof_ = false;
    }
private:
    dictionary&             dictionary_;
    std::string             current_word_;
    std::string::size_type  off_;
    bool                    eof_;
};

class dictionary_output_filter : public output_filter {
public:
    typedef std::map<std::string, std::string> map_type;
    dictionary_output_filter(dictionary& d) 
        : dictionary_(d), off_(std::string::npos)
        { }

    template<typename Sink>
    bool put(Sink& dest, int c)
    {   
        if (off_ != std::string::npos && !write_current_word(dest))
            return false;
                
        if (!std::isalpha(c, dictionary_.getloc())) {
            dictionary_.replace(current_word_);
            off_ = 0;
        }
            
        current_word_ += c;
        return true;
    }

    template<typename Sink>
    void close(Sink& dest) 
    { 
        if (off_ == std::string::npos)
            dictionary_.replace(current_word_);
        if (!current_word_.empty())
            write_current_word(dest);
        current_word_.clear();
        off_ = std::string::npos;
    }
private:
    template<typename Sink>
    bool write_current_word(Sink& dest) 
    { 
        using namespace std;
        streamsize amt = static_cast<streamsize>(current_word_.size() - off_);
        streamsize result = 
            iostreams::write(dest, current_word_.data() + off_, amt);
        if (result == amt) {
            current_word_.clear();
            off_ = string::npos;
            return true;
        } else {
            off_ += result;
            return false;
        }
    }

    dictionary&             dictionary_;
    std::string             current_word_;
    std::string::size_type  off_;
    bool                    initialized_;
};
                    
//------------------Implementation of dictionary------------------------------//

inline dictionary::dictionary(const std::locale& loc) : loc_(loc) { }

inline void dictionary::add(std::string key, const std::string& value)
{
    max_length_ = max_length_ < key.size() ? key.size() : max_length_;
    tolower(key);
    map_[key] = value;
}

inline bool dictionary::replace(std::string& key)
{
    using namespace std;
    string copy(key);
    tolower(copy);
    map_type::iterator it = map_.find(key);
    if (it == map_.end())
        return false;
    string& value = it->second;
    if (!value.empty() && !key.empty() && std::isupper(key[0], loc_))
        value[0] = std::toupper(value[0], loc_);
    key = value;
    return true;
}

inline const std::locale& dictionary::getloc() const { return loc_; }

inline void dictionary::tolower(std::string& str)
{
    for (std::string::size_type z = 0, len = str.size(); z < len; ++z)
        str[z] = std::tolower(str[z], loc_);
}

} } }       // End namespaces example, iostreams, boost.

#endif      // #ifndef BOOST_IOSTREAMS_DICTIONARY_FILTER_HPP_INCLUDED