//
//  Cache.hpp
//  LDParse
//
//  Created by Thomas Dickerson on 1/18/16.
//  Copyright © 2016 StickFigure Graphic Productions. All rights reserved.
//

#ifndef Cache_h
#define Cache_h

#include <algorithm>
#include <string>
#include <set>
#include <locale>
#include <iostream>
#include <boost/optional.hpp>

namespace LDParse {
	namespace Cache {
		template<typename Contents> class CacheNode {
		private:
			/*
			struct STLDereferencer
			{
				
				template <typename PtrType>
				bool operator()(PtrType pT1, PtrType pT2) const
				{
					return *pT1 < *pT2;
				}
			};
			 */
			
			struct STLDeleter
			{
				template <typename T>
				void operator()(const T* ptr) const
				{
					delete ptr;
				}
			};
			
			boost::optional<Contents> mContents;
			
			std::string mPrefix;
			std::set<std::unique_ptr<CacheNode>/*, STLDereferencer*/> mSuffixes;
			
		public:
			
			boost::optional<CacheNode&> find(std::string nodeName, int depth=-1) const {
				std::string ds("");
				if(depth >= 0){
					depth++;
					ds = std::string(depth,' ');
				}
				std::string commonPrefix = findCommonPrefix(mPrefix,nodeName);
				int cpS = commonPrefix.size();
				int nnS = nodeName.size();
				int mpS = mPrefix.size();
				boost::optional<CacheNode&> retNode = boost::none;
				if(depth >= 0){
					std::cout << ds << "Looking for " << nodeName << ", mPrefix is \"" << mPrefix << "\"" << std::endl;
					std::cout << ds << "- Node has children - " << std::endl;
					for(auto it = mSuffixes.begin(); it != mSuffixes.end() /* && (*it) != nullptr */; it++){
						std::cout << ds << "-- --- " << (*it)->mPrefix << std::endl;
					}
					
				}
				if(nnS == mpS && mpS == cpS){ // Same
					if(depth >= 0) std::cout << ds << " - Found it" << std::endl;
					retNode = this;
				} else if((cpS == nnS) || mSuffixes.size() == 0){ // Not in the tree.
					if(depth >= 0) std::cout << ds << " - the search is the full common prefix, but we don't match this" << std::endl;
					retNode = boost::none;
				} else {
					std::string suffix = nodeName.substr(cpS, nnS - cpS);
					
					retNode = boost::none;
					
					for(auto checkNode = mSuffixes.begin();	checkNode != mSuffixes.end() && (*checkNode) != nullptr; checkNode++) {
						
						if(findCommonPrefix((*checkNode)->mPrefix,suffix) != ""){
							if(depth >= 0) std::cout << ds << " - found matching prefix \"" << (*checkNode)->mPrefix << "\"" << std::endl;
							retNode = (*checkNode)->find(suffix,depth);
							break;
						}
					}
				}
				
				return retNode;
			}
			
			CacheNode& insert(std::string nodeName, Contents contents, int depth = -1) {
				std::string ds;
				if(depth >= 0){
					depth++;
					ds = std::string(depth,' ');
				}
				std::string commonPrefix = findCommonPrefix(mPrefix,nodeName);
				std::string curSuffix;
				AssertFatal(commonPrefix != "" || mPrefix == "", "We have arrived at a non-root node, and the common prefix is empty. This should never happen");
				int cpS = commonPrefix.size();
				int nnS = nodeName.size();
				int mpS = mPrefix.size();
				if((mpS == 0 && cpS == 0) || (cpS < nnS && cpS == mpS)){ // ("","something") or ("cat","catsup")
					curSuffix = nodeName.substr(cpS, nnS - cpS);
					std::unique_ptr<CacheNode> sufNode(new CacheNode(curSuffix));
					CacheNode * checkNode = nullptr;
					for(auto checkIt = mSuffixes.begin(); checkIt != mSuffixes.end() && (*checkIt) != nullptr; checkIt++) {
						checkNode = *checkIt;
						if(findCommonPrefix(curSuffix,checkNode->mPrefix) == "") checkNode = nullptr;
						else break;
					}
					if(checkNode == nullptr){
						mSuffixes.insert(sufNode);
						sufNode->setContents(contents);
						return *sufNode;
					} else {
						// sufNode will delete itself, because yay unique_ptr
						return checkNode->insert(curSuffix, contents, depth);
					}
					
				} else if(nnS == cpS && cpS == mpS){ // ("catsup","catsup")
					// AssertFatal(mContents == NULL || contents == NULL,"Overwriting LDCacheNode contents. This shouldn't happen, and the error is with whoever thought the same file needed to be inserted twice (see callstack).");
					if(contents != nullptr){
						if(mContents != nullptr) std::cerr << "CacheNode::insert() - Overwriting CacheNode contents. This should never happen - try running in DEBUG mode." << std::endl;
						mContents = contents;
					}
					return *this;
				} else if (cpS > 0 && cpS < mpS){ // ("catsup","cat")
												  // We split!
					curSuffix = mPrefix.substr(cpS, mpS - cpS);
					std::unique_ptr<CacheNode> newChild(new CacheNode(curSuffix, mContents));
					newChild->mSuffixes = mSuffixes;
					
					mSuffixes.clear();
					mSuffixes.insert(newChild);
					mPrefix = commonPrefix;
					if(nnS == cpS){
						mContents = contents;
						return *this;
					} else{
						curSuffix = nodeName.substr(cpS, nnS - cpS);
						std::unique_ptr<CacheNode> sufNode = new CacheNode(curSuffix, contents);
						mSuffixes.insert(sufNode);
						return *sufNode;
					}
				} else{ // WTF?
					std::cerr << "CacheNode::insert() - This is a peculiar circumstance. Try running in DEBUG mode" << std::endl;
					std::cerr << "CacheNode::insert() - node has " << mPrefix << ", trying to insert " << nodeName << " (common prefix " << commonPrefix << ")" << std::endl;
					std::cerr << "CacheNode::insert() - Node has children - " << std::endl;
					for(auto it = mSuffixes.begin(); it != mSuffixes.end() && (*it) != nullptr; it++){
						std::cerr << "CacheNode::insert() --- " << (*it)->mPrefix << std::endl;
					}
					//AssertFatal(false, "CacheNode::insert() - This is a peculiar circumstance. Please inspect me in the Debugger");
				}
			}
			
			
			void dump(std::ostream &out = std::cout, std::string prefix = ""){
				std::string withme(prefix);
				withme += mPrefix;
				out << withme << std::endl;
				for(auto it = mSuffixes.begin(); it != mSuffixes.end(); it++){
					it->dump(out, withme);
				}
			}
			
			void setContents(Contents newContents){mContents = newContents;}
			Contents getContents() const {return mContents;};
			
			static std::string findCommonPrefix(std::string str1, std::string str2){
				static std::locale fcpLocale;
				static char c1, c2;
				std::string prefix = std::string("");
				prefix.clear();
				size_t iMax = std::min(str1.size(), str2.size());
				for(size_t i = 0; i < iMax; i++){
					c1 = toupper(str1[i],fcpLocale);
					c2 = toupper(str2[i],fcpLocale);
					if(c1==c2) prefix.append(std::string(1,c1));
					else break;
				}
				return prefix;
			}
			
			static CacheNode* makeRoot(){ return new CacheNode; }
			
			~CacheNode();
			CacheNode(std::string prefix="", boost::optional<Contents> contents = boost::none)
			: mPrefix(prefix), mContents(contents) {
				mSuffixes.clear();
			}
			
			const bool operator<(const CacheNode &other) const {	return up(mPrefix) < up(other.mPrefix);	}
			const bool operator>(const CacheNode &other) const {	return up(mPrefix) > up(other.mPrefix);	}
			const bool operator<=(const CacheNode &other) const {	return up(mPrefix) <= up(other.mPrefix);	}
			const bool operator>=(const CacheNode &other) const {	return up(mPrefix) >= up(other.mPrefix);	}
			const bool operator==(const CacheNode &other) const {	return up(mPrefix) == up(other.mPrefix);	}
			const bool operator!=(const CacheNode &other) const {	return up(mPrefix) != up(other.mPrefix);	}
			
			inline const static std::string up(std::string str1) {
				static std::locale fcpLocale;
				std::string upper = str1;
				size_t iMax = str1.size();
				for(size_t i = 0; i < iMax; i++){
					upper[i] = std::toupper(str1[i],fcpLocale);
				}
				return upper;
			}
		};

	}
}

#endif /* Cache_h */
