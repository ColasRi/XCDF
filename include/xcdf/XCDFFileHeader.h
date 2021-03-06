
/*
Copyright (c) 2014, University of Maryland
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef XCDF_FILE_HEADER_INCLUDED_H
#define XCDF_FILE_HEADER_INCLUDED_H

#include <xcdf/XCDFFieldDescriptor.h>
#include <xcdf/alias/XCDFAliasDescriptor.h>
#include <xcdf/XCDFDefs.h>

#include <vector>
#include <string>
#include <cassert>
#include <algorithm>

/// Place descriptors in type order
inline
bool operator<(const XCDFFieldDescriptor& d1, const XCDFFieldDescriptor& d2) {
  return d1.type_ < d2.type_;
}

class XCDFFileHeader {

  public:

    XCDFFileHeader() : fileTrailerPtr_(0),
                       version_(XCDF_VERSION) { }

    ~XCDFFileHeader() { }

    void SetVersion(const unsigned version) {version_ = version;}
    uint32_t GetVersion() const {return version_;}

    void SetFileTrailerPtr(const uint64_t ptr) {fileTrailerPtr_ = ptr;}
    uint64_t GetFileTrailerPtr() const {return fileTrailerPtr_;}
    bool HasFileTrailerPtr() const {return fileTrailerPtr_ > 0;}

    void Clear() {fieldDescriptors_.clear();}

    /// Place descriptors in type order
    void AddFieldDescriptor(const XCDFFieldDescriptor& d) {
      fieldDescriptors_.insert(
                    std::upper_bound(fieldDescriptors_.begin(),
                                     fieldDescriptors_.end(), d), d);
    }


    bool HasAliasDescriptor(const XCDFAliasDescriptor& d) {
      return std::find(aliasDescriptors_.begin(),
                       aliasDescriptors_.end(), d) != aliasDescriptors_.end();
    }

    void AddAliasDescriptor(const XCDFAliasDescriptor& d) {
      if (!HasAliasDescriptor(d)) {
        aliasDescriptors_.push_back(d);
      }
    }

    std::vector<XCDFFieldDescriptor>::const_iterator
    FieldDescriptorsBegin() const {return fieldDescriptors_.begin();}

    std::vector<XCDFFieldDescriptor>::const_iterator
    FieldDescriptorsEnd() const {return fieldDescriptors_.end();}

    std::vector<XCDFAliasDescriptor>::const_iterator
    AliasDescriptorsBegin() const {return aliasDescriptors_.begin();}

    std::vector<XCDFAliasDescriptor>::const_iterator
    AliasDescriptorsEnd() const {return aliasDescriptors_.end();}

    uint32_t GetNFieldDescriptors() const {return fieldDescriptors_.size();}
    uint32_t GetNAliasDescriptors() const {return aliasDescriptors_.size();}

    void UnpackFrame(XCDFFrame& frame) {

      Clear();

      assert(frame.GetType() == XCDF_FILE_HEADER);

      version_ = frame.GetUnsigned32();
      if (version_ > XCDF_VERSION) {
        XCDFFatal("Unable to read file of version " << version_);
      }
      fileTrailerPtr_ = frame.GetUnsigned64();

      uint32_t nFields = frame.GetUnsigned32();
      fieldDescriptors_.reserve(nFields);
      XCDFFieldDescriptor descriptor;
      for (unsigned i = 0; i < nFields; ++i) {
        descriptor.name_ = frame.GetString();
        descriptor.type_ = frame.GetChar();
        descriptor.rawResolution_ = frame.GetUnsigned64();
        descriptor.parentName_ = frame.GetString();
        fieldDescriptors_.push_back(descriptor);
      }

      if (version_ > 2) {
        uint32_t nAliases = frame.GetUnsigned32();
        XCDFAliasDescriptor descriptor;
        for (unsigned i = 0; i < nAliases; ++i) {
          descriptor.SetName(frame.GetString());
          descriptor.SetExpression(frame.GetString());
          descriptor.SetType(XCDFFieldType(frame.GetChar()));
          aliasDescriptors_.push_back(descriptor);
        }
      }
    }

    void PackFrame(XCDFFrame& frame) const {

      frame.Clear();
      frame.SetType(XCDF_FILE_HEADER);
      frame.PutUnsigned32(version_);
      frame.PutUnsigned64(fileTrailerPtr_);

      frame.PutUnsigned32(fieldDescriptors_.size());
      for (std::vector<XCDFFieldDescriptor>::const_iterator
                         it = fieldDescriptors_.begin();
                         it != fieldDescriptors_.end(); ++it) {
        frame.PutString(it->name_);
        frame.PutChar(it->type_);
        frame.PutUnsigned64(it->rawResolution_);
        frame.PutString(it->parentName_);
      }

      frame.PutUnsigned32(aliasDescriptors_.size());
      for (std::vector<XCDFAliasDescriptor>::const_iterator
                         it = aliasDescriptors_.begin();
                         it != aliasDescriptors_.end(); ++it) {
        frame.PutString(it->GetName());
        frame.PutString(it->GetExpression());
        frame.PutChar(it->GetType());
      }
    }

    bool operator==(const XCDFFileHeader& fh) const {

      // Aliases do not affect the equivalence of headers
      return version_ == fh.version_ &&
             fieldDescriptors_ == fh.fieldDescriptors_;
    }

    bool operator!=(const XCDFFileHeader& fh) const {return !(*this == fh);}

  private:

    uint64_t fileTrailerPtr_;
    uint32_t version_;
    std::vector<XCDFFieldDescriptor> fieldDescriptors_;
    std::vector<XCDFAliasDescriptor> aliasDescriptors_;
};

#endif // XCDF_FILE_HEADER_INCLUDED_H
