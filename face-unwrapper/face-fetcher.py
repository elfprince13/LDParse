#!/usr/bin/env python3

from urllib.request import urlopen, Request
from urllib.parse import urlencode
import os
from bs4 import BeautifulSoup
import re

import time
import json

import csv

headers = {'User-Agent':"Mozilla/5.0 (X11; U; Linux i686) Gecko/20071127 Firefox/2.0.0.11"}
encoding = re.compile(r"^Content-Type: text/html;\s*charset=(.*)$",flags=re.MULTILINE | re.IGNORECASE)
json_encoding = re.compile(r"^Content-Type: application/json;\s*charset=(.*)$",flags=re.MULTILINE | re.IGNORECASE)
colorID = re.compile(r"colorID=([0-9]+)")
partID = re.compile(r"P=([0-9a-z]+)",flags=re.IGNORECASE)
colorCode = re.compile(r"background-color: #([0-9a-fA-F]{6})")
brickLinkID = re.compile(r"^0 !KEYWORDS .*Bricklink.+(3626[a-z0-9]+)",flags=re.MULTILINE | re.IGNORECASE)
setIDs = re.compile(r"(?<![0-9a-z])set\s+([a-z0-9]+)", flags=re.IGNORECASE)
allKeyWords = re.compile(r"^0 !KEYWORDS (.+)",flags=re.MULTILINE | re.IGNORECASE)
numericPrefix = re.compile(r"^([0-9]+).*")
colorMatcher = re.compile(r"^0\s+// LEGOID\s+(\d+) - ([a-zA-Z ]+)\s*\n0 !COLOUR\s+([a-zA-Z_]+)\s+CODE\s+(\d+)",re.MULTILINE | re.IGNORECASE)

def extractBestMatch(results, keyWords, nameNum, returnScore = False):
	bestResult = 0
	bestName = None
	for foundName, foundKeyWords in results:
		foundPref = numericPrefix.match(foundName)
		if foundPref:
			foundNum =int(foundPref.group(1))
			overlap = foundKeyWords & keyWords
			thisResult = len(overlap)
			if foundNum == nameNum:
				print(foundName, foundNum, thisResult, overlap)
				if thisResult > bestResult:
					bestResult = thisResult
					bestName = foundName
	if returnScore:
		return (bestName, bestResult)
	else:
		return bestName

def extractInventoryResults(htmlSrc):
	soup = BeautifulSoup(htmlSrc,"lxml")
	items = soup.find_all("tr","IV_ITEM")
	table = [tr.find_all("td") for tr in items]
	names = [a.get_text() for tr in table for a in tr[2].find_all("a") if a.get_text() != "Inv"]
	texts = [txtToKeyWords(tr[3].get_text()) for tr in table]
	return {name : text for name, text in zip(names, texts)}#extractBestMatch(zip(names, texts), keyWords, nameNum, returnScore)

def extractBestSearchMatch(jsonSrc, keyWords, nameNum, returnScore = False):
	data = json.loads(jsonSrc)
	try:
		data = data['result']['typeList'][0]['items']
		results = [(result['strItemNo'],txtToKeyWords(result['strItemName'].replace("\n","")))
			for result in data if result['typeItem'].lower() == 'p']
		return extractBestMatch(results, keyWords, nameNum, returnScore)
	except LookupError:
		print(data)
		if returnScore:
			return (None, 0)
		else:
			return None
	

def extractKnownColors(html):
	soup = BeautifulSoup(html,"lxml")
	color_table = soup.find("table","pciColorInfoTable")
	color_entries = color_table.find_all("td")[-1]
	return [(colorID.search(a.get("href")).group(1),
			 a.get_text(),
			 colorCode.search(span.get("style")).group(1))
		for (a, span) in zip(color_entries.find_all("a"),
							 color_entries.find_all("span","pciColorTabListItem"))]

inventory_url_prefixes = {"https://www.bricklink.com/CatalogItemInv.asp?S".lower(),
						  "http://www.bricklink.com/CatalogItemInv.asp?S=".lower()}
def extractInventoryURL(html):
	soup = BeautifulSoup(html,"lxml")
	partOut = soup.find(id="_idINVOptionTable")
	links = partOut.find_all("a")
	for link in links:
		href = link.get("href")
		if href and len(href) >= 46 and href[:46].lower() in inventory_url_prefixes:
			newLink = link.get("href")+"&bt=2"
			return newLink.replace("http://","https://")
	return None

bl_direct_url = "https://www.bricklink.com/v2/catalog/catalogitem.page?%s"
bl_search_url = "https://www.bricklink.com/ajax/clone/search/searchproduct.ajax?%s"

wait_for = 0.5
def fetchPartPage(partName):
	global wait_for
	partURL = bl_direct_url % urlencode({'P':partName})
	with urlopen(Request(partURL,headers=headers)) as bl_result:
			status = bl_result.getcode()
			if bl_result.geturl() == partURL:
				if status == 200:
					info = bl_result.info()
					charset = encoding.search(str(info)).group(1)
					return bl_result.read().decode(charset)
				else:
					print("HTTP Status %d" % status)
					wait_for *= 2
					print("Backing off %f" % wait_for)
					time.sleep(wait_for)
			else:
				print("Was redirected to %s" % bl_result.geturl())
				if bl_result.getcode() >= 400:		
					print("HTTP Status %d" % status)
					wait_for *= 2
					print("Backing off %f" % wait_for)
					time.sleep(wait_for)
	return None
def fetchSetPage(setName,useNameAsURL=False):
	global wait_for
	setURL = setName if useNameAsURL else bl_direct_url % urlencode({'S':setName})
	with urlopen(Request(setURL,headers=headers)) as bl_result:
			status = bl_result.getcode()
			if bl_result.geturl() == setURL:
				if status == 200:
					info = bl_result.info()
					charset = encoding.search(str(info)).group(1)
					return bl_result.read().decode(charset)
				else:
					print("HTTP Status %d" % status)
					wait_for *= 2
					print("Backing off %f" % wait_for)
					time.sleep(wait_for)
			else:
				print("Was redirected to %s" % bl_result.geturl())
				if bl_result.getcode() >= 400:		
					print("HTTP Status %d" % status)
					wait_for *= 2
					print("Backing off %f" % wait_for)
					time.sleep(wait_for)
	return None

def fetchSearchPage(searchTerms):
	searchURL = bl_search_url %  urlencode({'q': searchTerms, 'rpp':500})
	print(searchURL)
	global wait_for
	with urlopen(Request(searchURL,headers=headers)) as bl_result:
			status = bl_result.getcode()
			if bl_result.geturl() == searchURL:
				if status == 200:
					info = bl_result.info()
					charset = json_encoding.search(str(info)).group(1)
					return bl_result.read().decode(charset)
				else:
					print("HTTP Status %d" % status)
					wait_for *= 2
					print("Backing off %f" % wait_for)
					time.sleep(wait_for)
			else:
				print("Was redirected to %s" % bl_result.geturl())
				if bl_result.getcode() >= 400:
					print("HTTP Status %d" % status)
					wait_for *= 2
					print("Backing off %f" % wait_for)
					time.sleep(wait_for)
	return None
					
ldraw2bricklink = {"2-sided" : "Dual Sided", "2sided" : "Dual Sided", "minifig" : "Minifigure", "grey" : "Gray"}

def cleanLine(line):
	line = line.replace("(","")
	line = line.replace(")","")
	line = line.replace("-","")
	line = line.replace("/","")
	line = line.replace(",","")
	line = line.replace("~","")
	line = line.replace("\xa0"," ")
	return line

def txtToKeyWords(line, asSet = True, brickLinked = True, exclusions = set()):
	keyWordsGenerator = (word
						 for phrase in cleanLine(line).lower().split(" ")
						 for word in (ldraw2bricklink.get(phrase, phrase).lower() if brickLinked else (phrase,)).split(" ")
						 if word and word not in exclusions)
	if asSet:
		return set(keyWordsGenerator)
	else:
		return list(keyWordsGenerator)

def main(faces, colortable, docFreqs, output, begin = 0):
	colortable_backup = {k.replace("_"," ").lower() : v for _, (_, v, k) in colortable.items()}
	numFaces = len(faces)
	with open(output,'a',newline='') as out_handle:
		writer = csv.writer(out_handle)
		for i,face in enumerate(faces[begin:],begin):
			print("Acquiring data for %d / %d" % ((i+1),numFaces))
			name = os.path.basename(face)
			name = name.split(".")[0]
			nameNum = int(numericPrefix.match(name).group(1))
			print(name)
			out_line = [face, name, None, None, None, None, None, None]
			
			model_lines=[]
			with open(face,'r') as model_handle:
				model = model_handle.read()
				model_lines = model.splitlines()
			if (model_lines[0][:11] == "0 ~Moved to") or (cleanLine(model_lines[0])[-8:].lower() == "obsolete"):
				print("\tSkipping defunct part %s" % name)
				continue
			elif "2-Sided" in model_lines[0]:
				out_line[5] = "*"
				print("\t2-Sided")
			data = fetchPartPage(name)
			if data:
				out_line[2] = name
				out_line[3] = extractKnownColors(data)
			else:
				maybeName = brickLinkID.search(model)
				if maybeName:
					name = maybeName.group(1)
					print("Model source specifies Bricklink %s" % name)
					data = fetchPartPage(name)
					if data:
						out_line[2] = name
						out_line[3] = extractKnownColors(data)
				else:
					nameLine = model.splitlines()[0]
					nameWords = txtToKeyWords(nameLine[2:], False, True, {"pattern"})
					nameLine = " ".join(nameWords)
					
					if len(nameLine) > 75:
						cutAt = nameLine.rfind(" ",0,75)
						nameLine = nameLine[:cutAt]
					
					allWords = [word for word in nameWords]
					keyWords = set(nameWords)
					sets = set()
					for kwResult in allKeyWords.findall(model):
						sets |= set(setIDs.findall(kwResult))
						moreKeyWords = txtToKeyWords(kwResult, False)
						allWords += moreKeyWords
						keyWords |= set(moreKeyWords)
					if len(sets):
						print("Appears in sets %s" % str(sets))
						shared_inventory = {}
						for setID in sets:
							data = fetchSetPage(setID)
							if data:
								inventoryURL = extractInventoryURL(data)
								if inventoryURL:
									data = fetchSetPage(inventoryURL, True)
									if data:
										parts = extractInventoryResults(data)
										if parts:
											if shared_inventory:
												remove_parts = {k for k in shared_inventory.keys() if k not in parts}
												for k in remove_parts:
													del shared_inventory[k]
											else:
												shared_inventory = parts
						print("intersection over parts from extant sets: %s" % str(shared_inventory.keys()))
						bestName = extractBestMatch(shared_inventory.items(), keyWords, nameNum)
						if bestName:
							data = fetchPartPage(bestName)
							if data:
								print(bestName)
								out_line[2] = bestName
								out_line[3] = extractKnownColors(data)
							
					if not out_line[2]:
						print(keyWords)
							
						searchData = fetchSearchPage(nameLine)
						if searchData:
							bestName = extractBestSearchMatch(searchData, keyWords, nameNum)
							if bestName:
								data = fetchPartPage(bestName)
								if data:
									out_line[2] = bestName
									out_line[3] = extractKnownColors(data)
							else:
								# hail mary, try to guess a unique identifier
								termFreqs = calculate_term_frequency(allWords)
								termRanks = sorted((termFreqs[word]/float(docFreqs[word]), word) for word in keyWords)
								print(termRanks)
								testNext = 1
								bestName = None
								while testNext <= len(termRanks) and termRanks[-testNext][0] > 0.1:
									time.sleep(0.5)
									nextTerm = termRanks[-testNext][1]
									try:
										int(nextTerm)
									except ValueError:
										if len(nextTerm) > 2:
											searchData = fetchSearchPage(["minifigure","head",nextTerm])
											if searchData:
												bestName = extractBestSearchMatch(searchData, keyWords, nameNum)
												if bestName:
													data = fetchPartPage(bestName)
													if data:
														out_line[2] = bestName
														out_line[3] = extractKnownColors(data)
														break
									testNext += 1
			if out_line[3]:
				out_line[4] = []
				for (code, name, _) in out_line[3]:
					greyed = name.replace("Gray","Grey")
					if greyed.lower() in colortable_backup:
							out_line[4].append((colortable_backup[greyed.lower()],greyed.replace(" ","_")))
					else:
						print("No fall-back available for %s, send help" % greyed.lower())
						out_line[6] = "*"
			if not out_line[2]:
				out_line[6] = "*"
			writer.writerow(out_line)
			time.sleep(0.5)
			if wait_for > 4:
				print("back off too long, stopping after %d",i)
				break
			
def calculate_document_frequency(documents):
	occurrences = {}
	for path in documents:
		with open(path, 'r') as doc_handle:
			document = doc_handle.read()
			keyWords = txtToKeyWords(document.splitlines()[0][1:])
			for kwResult in allKeyWords.findall(document):
				keyWords |= txtToKeyWords(kwResult)
			for keyWord in keyWords:
				occurrences[keyWord] = 1 + occurrences.get(keyWord, 0)
	return occurrences

def calculate_term_frequency(allWords):
	occurrences = {}
	for word in allWords:
		occurrences[word] = 1 + occurrences.get(word, 0)
	return occurrences
		
if __name__ == '__main__':
	import sys
	data = []
	with open(sys.argv[1],'r') as files_list:
		data = files_list.read().splitlines()
	document_frequencies = calculate_document_frequency(data)
	colorTable = {}
	with open(sys.argv[2],'r') as ldconfig:
		colorData = ldconfig.read()
		colorData = colorMatcher.findall(colorData)
		colorTable = {blCode : (blName, ldCode, ldName) for (blCode, blName, ldName, ldCode) in colorData}
	outfile = sys.argv[3]
	start = sys.argv[4] if len(sys.argv) > 4 else 0
	main(data, colorTable, document_frequencies, outfile, start)