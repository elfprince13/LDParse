//
//  ReadFs.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/17/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#ifndef ReadFs_h
#define ReadFs_h

#include "Lex.hpp"
#include "Expect.hpp"

namespace LDParse {

	template<typename ErrF> bool readColor(TokenStream::const_iterator &tokenIt, ColorRef &color, const FailF<ErrF> &fail) {
		bool ret = true;
		switch(tokenIt->k){
			case HexInt:
				color = {false, boost::get<int32_t>((tokenIt++)->v)};
				break;
			case Zero...Five:
			case DecInt:
				color = {true, boost::get<int32_t>((tokenIt++)->v)};
				break;
			default:
				fail(tokenIt, &ret);
		}
		return ret;
	}
	
	template<typename ErrF> bool readNumber(TokenStream::const_iterator &tokenIt, float &num, const FailF<ErrF> &fail) {
		bool ret = true;
		switch(tokenIt->k){
			case HexInt:
			case DecInt:
			case Zero...Five:
				num = boost::get<int32_t>((tokenIt++)->v);
				break;
			case Float:
				num = boost::get<float>((tokenIt++)->v);
				break;
			default:
				fail(tokenIt, &ret);
		}
		return ret;
	}
	
	template<typename ErrF> bool readIdent(TokenStream::const_iterator &tokenIt, std::string &id, const FailF<ErrF> &fail)  {
		bool ret = true;
		switch(tokenIt->k){
			case Ident:
				id = boost::get<std::string>((tokenIt++)->v);
				break;
			default:
				fail(tokenIt, &ret);
		}
		return ret;
	}
	
	template<typename ErrF> bool readPosition(TokenStream::const_iterator &tokenIt, Position &p, const FailF<ErrF> &fail) {
		const TokenStream::const_iterator start = tokenIt;
		bool ret = true;
		ret = readNumber(tokenIt, std::get<0>(p), fail)
		&&readNumber(tokenIt, std::get<1>(p), fail)
		&&readNumber(tokenIt, std::get<2>(p), fail);
		if(!ret) fail(start, &ret);
		return ret;
	}
	
	template<typename ErrF> bool readLine(TokenStream::const_iterator &tokenIt, Line &l, const FailF<ErrF> &fail) {
		const TokenStream::const_iterator start = tokenIt;
		bool ret = true;
		ret = readPosition(tokenIt, std::get<0>(l), fail)
		&&readPosition(tokenIt, std::get<1>(l), fail);
		if(!ret) fail(start, &ret);
		return ret;
	}
	
	template<typename ErrF> bool readTriangle(TokenStream::const_iterator &tokenIt, Triangle &t, const FailF<ErrF> &fail) {
		const TokenStream::const_iterator start = tokenIt;
		bool ret = true;
		ret = readPosition(tokenIt, std::get<0>(t), fail)
		&&readPosition(tokenIt, std::get<1>(t), fail)
		&&readPosition(tokenIt, std::get<2>(t), fail);
		if(!ret) fail(start, &ret);
		return ret;
	}
	
	template<typename ErrF> bool readQuad(TokenStream::const_iterator &tokenIt, Quad &q, const FailF<ErrF> &fail) {
		const TokenStream::const_iterator start = tokenIt;
		bool ret = true;
		ret = readPosition(tokenIt, std::get<0>(q), fail)
		&&readPosition(tokenIt, std::get<1>(q), fail)
		&&readPosition(tokenIt, std::get<2>(q), fail)
		&&readPosition(tokenIt, std::get<3>(q), fail);
		if(!ret) fail(start, &ret);
		return ret;
	}
	
	template<typename ErrF> bool readOptLine(TokenStream::const_iterator &tokenIt, OptLine &o, const FailF<ErrF> &fail) {
		const TokenStream::const_iterator start = tokenIt;
		bool ret = true;
		ret = readLine(tokenIt, std::get<0>(o), fail)
		&&readLine(tokenIt, std::get<1>(o), fail);
		if(!ret) fail(start, &ret);
		return ret;
	}
	
	template<typename ErrF> bool readMat(TokenStream::const_iterator &tokenIt, TransMatrix &m, const FailF<ErrF> &fail) {
		const TokenStream::const_iterator start = tokenIt;
		bool ret = true;
		ret = readPosition(tokenIt, std::get<0>(m), fail)
		&&readNumber(tokenIt, std::get<1>(m), fail)
		&&readNumber(tokenIt, std::get<2>(m), fail)
		&&readNumber(tokenIt, std::get<3>(m), fail)
		&&readNumber(tokenIt, std::get<4>(m), fail)
		&&readNumber(tokenIt, std::get<5>(m), fail)
		&&readNumber(tokenIt, std::get<6>(m), fail)
		&&readNumber(tokenIt, std::get<7>(m), fail)
		&&readNumber(tokenIt, std::get<8>(m), fail)
		&&readNumber(tokenIt, std::get<9>(m), fail);
		if(!ret) fail(start, &ret);
		return ret;
	}
	
}

#endif /* ReadFs_h */
