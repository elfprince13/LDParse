//
//  Expect.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/17/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#ifndef Expect_h
#define Expect_h

#include "Lex.hpp"

namespace LDParse {
	
	template<typename  ErrHandler> class FailF{
	private:
		void failed(const std::string &expect, const TokenStream::const_iterator &tokenIt, bool * expectSuccess = nullptr) const {
			failed(expect, tokenIt->textRepr(), expectSuccess);
		}
		void failed(const std::string &expect, const std::string &found, bool * expectSuccess = nullptr) const {
			mErr("Expected " + expect, found, true);
			if(expectSuccess != nullptr) *expectSuccess = false;
		}
		
		const char * mTokenName;
		ErrHandler &mErr;
	public:
		FailF(ErrHandler &errH, const char * tokenName) : mTokenName(tokenName), mErr(errH) {}
		
		void operator()(const TokenStream::const_iterator &tokenIt, bool * expectSuccess = nullptr) const {
			failed(mTokenName, tokenIt, expectSuccess);
		}
		
		void operator()(const std::string &found, bool * expectSuccess = nullptr) const {
			failed(mTokenName, found, expectSuccess);
		}
	};
	
	template<typename Out, typename ErrHandler> using ReadF = bool (*)(TokenStream::const_iterator &tokenIt, Out &o, const FailF<ErrHandler> &fail);
	
	template<typename ReadF, typename Out, size_t tokenLen, const char * tokenName, typename ErrHandler> class Expect{
	private:
		ReadF mRead;
		ErrHandler &mErr;
		const FailF<ErrHandler> failF;
	public:
		constexpr static const size_t TokenCount = tokenLen;
		typedef Expect<ReadF, Out, tokenLen, tokenName, ErrHandler> SelfType;

		Expect(ErrHandler &err, ReadF read) : mRead(read), mErr(err), failF(mErr, tokenName) {}
		
		bool operator()(TokenStream::const_iterator &tokenIt, const TokenStream::const_iterator &eol, Out &o){
			bool ret = std::distance(tokenIt, eol) >= 1;
			if(ret) {
				ret = mRead(tokenIt, o, failF);
			} else {
				failF("EOL");
			}
			return ret;
		}
	};
	
	template<const char* tokenName, typename ErrHandler> class Expect<void, TokenStream::const_iterator, 0, tokenName, ErrHandler> {
	private:
		ErrHandler &mErr;
		const FailF<ErrHandler> failF;
	public:
		Expect(ErrHandler &err) : mErr(err), failF(mErr, tokenName) {}
		
		bool operator()(const TokenStream::const_iterator &tokenIt,const TokenStream::const_iterator &compare) const{
			bool ret = std::distance(tokenIt, compare) == 0;
			if(!ret) failF(tokenIt);
			return ret;
		}
	};
}

#endif /* Expect_h */
