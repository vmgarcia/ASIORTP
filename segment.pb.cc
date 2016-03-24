// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: segment.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "segment.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace rtp {

namespace {

const ::google::protobuf::Descriptor* Segment_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  Segment_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_segment_2eproto() {
  protobuf_AddDesc_segment_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "segment.proto");
  GOOGLE_CHECK(file != NULL);
  Segment_descriptor_ = file->message_type(0);
  static const int Segment_offsets_[7] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Segment, source_dest_port_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Segment, sequence_no_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Segment, ack_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Segment, syn_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Segment, fin_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Segment, receive_window_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Segment, data_),
  };
  Segment_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      Segment_descriptor_,
      Segment::default_instance_,
      Segment_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Segment, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Segment, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(Segment));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_segment_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    Segment_descriptor_, &Segment::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_segment_2eproto() {
  delete Segment::default_instance_;
  delete Segment_reflection_;
}

void protobuf_AddDesc_segment_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\rsegment.proto\022\003rtp\"\205\001\n\007Segment\022\030\n\020sour"
    "ce_dest_port\030\001 \001(\005\022\023\n\013sequence_no\030\002 \001(\005\022"
    "\013\n\003ack\030\003 \001(\010\022\013\n\003syn\030\004 \001(\010\022\013\n\003fin\030\005 \001(\010\022\026"
    "\n\016receive_window\030\006 \001(\005\022\014\n\004data\030\007 \001(\014", 156);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "segment.proto", &protobuf_RegisterTypes);
  Segment::default_instance_ = new Segment();
  Segment::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_segment_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_segment_2eproto {
  StaticDescriptorInitializer_segment_2eproto() {
    protobuf_AddDesc_segment_2eproto();
  }
} static_descriptor_initializer_segment_2eproto_;

// ===================================================================

#ifndef _MSC_VER
const int Segment::kSourceDestPortFieldNumber;
const int Segment::kSequenceNoFieldNumber;
const int Segment::kAckFieldNumber;
const int Segment::kSynFieldNumber;
const int Segment::kFinFieldNumber;
const int Segment::kReceiveWindowFieldNumber;
const int Segment::kDataFieldNumber;
#endif  // !_MSC_VER

Segment::Segment()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:rtp.Segment)
}

void Segment::InitAsDefaultInstance() {
}

Segment::Segment(const Segment& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:rtp.Segment)
}

void Segment::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  source_dest_port_ = 0;
  sequence_no_ = 0;
  ack_ = false;
  syn_ = false;
  fin_ = false;
  receive_window_ = 0;
  data_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Segment::~Segment() {
  // @@protoc_insertion_point(destructor:rtp.Segment)
  SharedDtor();
}

void Segment::SharedDtor() {
  if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete data_;
  }
  if (this != default_instance_) {
  }
}

void Segment::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Segment::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Segment_descriptor_;
}

const Segment& Segment::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_segment_2eproto();
  return *default_instance_;
}

Segment* Segment::default_instance_ = NULL;

Segment* Segment::New() const {
  return new Segment;
}

void Segment::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<Segment*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  if (_has_bits_[0 / 32] & 127) {
    ZR_(source_dest_port_, receive_window_);
    if (has_data()) {
      if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        data_->clear();
      }
    }
  }

#undef OFFSET_OF_FIELD_
#undef ZR_

  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool Segment::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:rtp.Segment)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional int32 source_dest_port = 1;
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &source_dest_port_)));
          set_has_source_dest_port();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(16)) goto parse_sequence_no;
        break;
      }

      // optional int32 sequence_no = 2;
      case 2: {
        if (tag == 16) {
         parse_sequence_no:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &sequence_no_)));
          set_has_sequence_no();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(24)) goto parse_ack;
        break;
      }

      // optional bool ack = 3;
      case 3: {
        if (tag == 24) {
         parse_ack:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &ack_)));
          set_has_ack();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(32)) goto parse_syn;
        break;
      }

      // optional bool syn = 4;
      case 4: {
        if (tag == 32) {
         parse_syn:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &syn_)));
          set_has_syn();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(40)) goto parse_fin;
        break;
      }

      // optional bool fin = 5;
      case 5: {
        if (tag == 40) {
         parse_fin:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &fin_)));
          set_has_fin();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(48)) goto parse_receive_window;
        break;
      }

      // optional int32 receive_window = 6;
      case 6: {
        if (tag == 48) {
         parse_receive_window:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &receive_window_)));
          set_has_receive_window();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(58)) goto parse_data;
        break;
      }

      // optional bytes data = 7;
      case 7: {
        if (tag == 58) {
         parse_data:
          DO_(::google::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_data()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:rtp.Segment)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:rtp.Segment)
  return false;
#undef DO_
}

void Segment::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:rtp.Segment)
  // optional int32 source_dest_port = 1;
  if (has_source_dest_port()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(1, this->source_dest_port(), output);
  }

  // optional int32 sequence_no = 2;
  if (has_sequence_no()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(2, this->sequence_no(), output);
  }

  // optional bool ack = 3;
  if (has_ack()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(3, this->ack(), output);
  }

  // optional bool syn = 4;
  if (has_syn()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(4, this->syn(), output);
  }

  // optional bool fin = 5;
  if (has_fin()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(5, this->fin(), output);
  }

  // optional int32 receive_window = 6;
  if (has_receive_window()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(6, this->receive_window(), output);
  }

  // optional bytes data = 7;
  if (has_data()) {
    ::google::protobuf::internal::WireFormatLite::WriteBytesMaybeAliased(
      7, this->data(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:rtp.Segment)
}

::google::protobuf::uint8* Segment::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:rtp.Segment)
  // optional int32 source_dest_port = 1;
  if (has_source_dest_port()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(1, this->source_dest_port(), target);
  }

  // optional int32 sequence_no = 2;
  if (has_sequence_no()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(2, this->sequence_no(), target);
  }

  // optional bool ack = 3;
  if (has_ack()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteBoolToArray(3, this->ack(), target);
  }

  // optional bool syn = 4;
  if (has_syn()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteBoolToArray(4, this->syn(), target);
  }

  // optional bool fin = 5;
  if (has_fin()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteBoolToArray(5, this->fin(), target);
  }

  // optional int32 receive_window = 6;
  if (has_receive_window()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(6, this->receive_window(), target);
  }

  // optional bytes data = 7;
  if (has_data()) {
    target =
      ::google::protobuf::internal::WireFormatLite::WriteBytesToArray(
        7, this->data(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:rtp.Segment)
  return target;
}

int Segment::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional int32 source_dest_port = 1;
    if (has_source_dest_port()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->source_dest_port());
    }

    // optional int32 sequence_no = 2;
    if (has_sequence_no()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->sequence_no());
    }

    // optional bool ack = 3;
    if (has_ack()) {
      total_size += 1 + 1;
    }

    // optional bool syn = 4;
    if (has_syn()) {
      total_size += 1 + 1;
    }

    // optional bool fin = 5;
    if (has_fin()) {
      total_size += 1 + 1;
    }

    // optional int32 receive_window = 6;
    if (has_receive_window()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->receive_window());
    }

    // optional bytes data = 7;
    if (has_data()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::BytesSize(
          this->data());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Segment::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const Segment* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const Segment*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void Segment::MergeFrom(const Segment& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_source_dest_port()) {
      set_source_dest_port(from.source_dest_port());
    }
    if (from.has_sequence_no()) {
      set_sequence_no(from.sequence_no());
    }
    if (from.has_ack()) {
      set_ack(from.ack());
    }
    if (from.has_syn()) {
      set_syn(from.syn());
    }
    if (from.has_fin()) {
      set_fin(from.fin());
    }
    if (from.has_receive_window()) {
      set_receive_window(from.receive_window());
    }
    if (from.has_data()) {
      set_data(from.data());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void Segment::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Segment::CopyFrom(const Segment& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Segment::IsInitialized() const {

  return true;
}

void Segment::Swap(Segment* other) {
  if (other != this) {
    std::swap(source_dest_port_, other->source_dest_port_);
    std::swap(sequence_no_, other->sequence_no_);
    std::swap(ack_, other->ack_);
    std::swap(syn_, other->syn_);
    std::swap(fin_, other->fin_);
    std::swap(receive_window_, other->receive_window_);
    std::swap(data_, other->data_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata Segment::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = Segment_descriptor_;
  metadata.reflection = Segment_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace rtp

// @@protoc_insertion_point(global_scope)
