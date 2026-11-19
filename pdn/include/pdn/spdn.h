#ifndef PDN_Header_spdn
#define PDN_Header_spdn

// The public umbrella header: include this to read and write spdn text.
//
// It carries the three pieces the documented API is built from: the entity container and
// its accessors (pdn_entity.h), the pdn::parse overload family (pdn_parse.h) and the
// pdn::make_uNserializer / pdn::serializer family (pdn_serializer.h). Including those
// three directly is equivalent and cheaper when only one is needed; the layers below
// (token, lexer, DFA, decoders) arrive transitively and stay internal.

#include "pdn_version.h"
#include "pdn_entity.h"
#include "pdn_parse.h"
#include "pdn_serializer.h"

#endif
