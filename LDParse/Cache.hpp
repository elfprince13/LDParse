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
#include <vector>
#include <locale>
#include <iostream>
#include <cassert>
#include <boost/optional.hpp>

namespace LDParse {
	namespace Cache {
		template<typename Contents> class CacheNode {
		private:
			std::unique_ptr<Contents> mContents;
			std::string mPrefix;
			std::vector<std::unique_ptr<CacheNode>> mSuffixes; // Semantically, this is a set, but we don't use any of the nice set operations, so...
			
			inline static const std::string findCommonPrefix(const std::string& str1, const std::string& str2){
				static std::locale fcpLocale;
				char c1, c2;
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
			
			inline const static std::string up(const std::string& str1) {
				static std::locale fcpLocale;
				std::string upper = str1;
				size_t iMax = str1.size();
				for(size_t i = 0; i < iMax; i++){
					upper[i] = std::toupper(str1[i],fcpLocale);
				}
				return upper;
			}
		public:
			boost::optional<Contents&> find(std::string nodeName, int depth=-1) const {
				std::string ds("");
				if(depth >= 0){
					depth++;
					ds = std::string(depth,' ');
				}
				std::string commonPrefix = findCommonPrefix(mPrefix,nodeName);
				size_t cpS = commonPrefix.size();
				size_t nnS = nodeName.size();
				size_t mpS = mPrefix.size();
				boost::optional<Contents&> retVal = boost::none;
				if(depth >= 0){
					std::cout << ds << "Looking for " << nodeName << ", mPrefix is \"" << mPrefix << "\"" << std::endl;
					std::cout << ds << "- Node has children - " << std::endl;
					for(auto it = mSuffixes.begin(); it != mSuffixes.end() /* && (*it) != nullptr */; it++){
						std::cout << ds << "-- --- " << (*it)->mPrefix << std::endl;
					}
					
				}
				if(nnS == mpS && mpS == cpS){ // Same
					if(depth >= 0) std::cout << ds << " - Found it" << std::endl;
					if(mContents) retVal = *mContents;
				} else if((cpS == nnS) || mSuffixes.size() == 0){ // Not in the tree.
					if(depth >= 0) std::cout << ds << " - the search is the full common prefix, but we don't match this" << std::endl;
				} else {
					std::string suffix = nodeName.substr(cpS, nnS - cpS);
					for(auto checkNode = mSuffixes.begin();	checkNode != mSuffixes.end() && (*checkNode) != nullptr; checkNode++) {
						
						if(findCommonPrefix((*checkNode)->mPrefix,suffix) != ""){
							if(depth >= 0) std::cout << ds << " - found matching prefix \"" << (*checkNode)->mPrefix << "\"" << std::endl;
							retVal = (*checkNode)->find(suffix,depth);
							break;
						}
					}
				}
				
				return retVal;
			}
			
			CacheNode& insert(std::string nodeName, std::unique_ptr<Contents> contents, int depth = -1) {
				std::string ds;
				if(depth >= 0){
					depth++;
					ds = std::string(depth,' ');
				}
				std::string commonPrefix = findCommonPrefix(mPrefix,nodeName);
				std::string curSuffix;
				assert((commonPrefix != "" || mPrefix == "") && "We have arrived at a non-root node, and the common prefix is empty. This should never happen");
				size_t cpS = commonPrefix.size();
				size_t nnS = nodeName.size();
				size_t mpS = mPrefix.size();
				CacheNode * retNode = nullptr;
				if((mpS == 0 && cpS == 0) || (cpS < nnS && cpS == mpS)){ // ("","something") or ("cat","catsup")
					curSuffix = nodeName.substr(cpS, nnS - cpS);
					std::unique_ptr<CacheNode> sufNode(new CacheNode(curSuffix));
					boost::optional<CacheNode&> checkNode = boost::none;
					for(auto checkIt = mSuffixes.begin(); checkIt != mSuffixes.end() && (*checkIt); checkIt++) {
						checkNode = **checkIt;
						if(findCommonPrefix(curSuffix,checkNode->mPrefix) == "") checkNode = boost::none;
						else break;
					}
					
					if(checkNode) {
						// sufNode will delete itself, because yay unique_ptr
						retNode = &(checkNode->insert(curSuffix, std::move(contents), depth));
					} else {
						sufNode->setContents(std::move(contents));
						retNode = sufNode.get();
						mSuffixes.push_back(std::move(sufNode));
					}
					
				} else if(nnS == cpS && cpS == mpS){ // ("catsup","catsup")
					assert((mContents == nullptr || contents == nullptr) && "Overwriting CacheNode contents. This shouldn't happen, and the error is with whoever thought the same file needed to be inserted twice (see callstack).");
					if(contents != nullptr){
						if(mContents != nullptr) std::cerr << "CacheNode::insert() - Overwriting CacheNode contents. This should never happen - try running in DEBUG mode." << std::endl;
						setContents(std::move(contents));
					}
					retNode = this;
				} else if (cpS > 0 && cpS < mpS){ // ("catsup","cat")
												  // We split!
					curSuffix = mPrefix.substr(cpS, mpS - cpS);
					std::unique_ptr<CacheNode> newChild(new CacheNode(curSuffix, std::move(mContents)));
					newChild->mSuffixes = std::move(mSuffixes);
					
					mSuffixes.clear();
					mSuffixes.push_back(std::move(newChild));
					mPrefix = commonPrefix;
					if(nnS == cpS){
						setContents(std::move(contents));
						retNode = this;
					} else{
						curSuffix = nodeName.substr(cpS, nnS - cpS);
						std::unique_ptr<CacheNode> sufNode(new CacheNode(curSuffix, std::move(contents)));
						retNode = sufNode.get();
						mSuffixes.push_back(std::move(sufNode));
					}
				} else{ // WTF?
					retNode = nullptr; // Just to be safe, make sure we really blow up.
					std::cerr << "CacheNode::insert() - This is a peculiar circumstance. Try running in DEBUG mode" << std::endl;
					std::cerr << "CacheNode::insert() - node has " << mPrefix << ", trying to insert " << nodeName << " (common prefix " << commonPrefix << ")" << std::endl;
					std::cerr << "CacheNode::insert() - Node has children - " << std::endl;
					for(auto it = mSuffixes.begin(); it != mSuffixes.end() && (*it) != nullptr; it++){
						std::cerr << "CacheNode::insert() --- " << (*it)->mPrefix << std::endl;
					}
					//AssertFatal(false, "CacheNode::insert() - This is a peculiar circumstance. Please inspect me in the Debugger");
				}
				return *retNode;
			}
			
			
			void dump(std::ostream &out = std::cout, std::string prefix = ""){
				std::string withme(prefix);
				withme += mPrefix;
				out << withme << std::endl;
				for(auto it = mSuffixes.begin(); it != mSuffixes.end(); it++){
					(*it)->dump(out, withme);
				}
			}
			
			void setContents(std::unique_ptr<Contents> newContents){ mContents = std::move(newContents); }
			boost::optional<Contents&> getContents() const {return mContents ? boost::optional<Contents&>(*mContents) : boost::none;};
			
			static CacheNode* makeRoot(){ return new CacheNode; }
			
			CacheNode(std::string prefix = "", std::unique_ptr<Contents> contents = nullptr) : mPrefix(prefix) {
				setContents(std::move(contents));
				mSuffixes.clear();
			}
		};

	}
}

#endif /* Cache_h */
