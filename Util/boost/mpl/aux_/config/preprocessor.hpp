
#ifndef BOOST_MPL_AUX_CONFIG_PREPROCESSOR_HPP_INCLUDED
#define BOOST_MPL_AUX_CONFIG_PREPROCESSOR_HPP_INCLUDED

// Copyright (c) 2000-04 Aleksey Gurtovoy
//
// Use, modification and distribution are subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy 
// at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Source: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/boost/mpl/aux_/config/preprocessor.hpp,v $
// $Date: 2005/02/17 23:25:13 $
// $Revision: 1.1 $

#include "boost/mpl/aux_/config/workaround.hpp"

#if !defined(BOOST_MPL_BROKEN_PP_MACRO_EXPANSION) \
    && (   BOOST_WORKAROUND(__MWERKS__, <= 0x3003) \
        || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x561)) \
        || BOOST_WORKAROUND(__IBMCPP__, BOOST_TESTED_AT(502)) \
        )

#   define BOOST_MPL_BROKEN_PP_MACRO_EXPANSION

#endif

//#define BOOST_MPL_NO_OWN_PP_PRIMITIVES

#if !defined(BOOST_NEEDS_TOKEN_PASTING_OP_FOR_TOKENS_JUXTAPOSING) \
    && BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x833))

#   define BOOST_NEEDS_TOKEN_PASTING_OP_FOR_TOKENS_JUXTAPOSING

#endif

#endif // BOOST_MPL_AUX_CONFIG_PREPROCESSOR_HPP_INCLUDED
