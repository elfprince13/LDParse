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
allKeyWords = re.compile(r"^0 !KEYWORDS (.+)",flags=re.MULTILINE | re.IGNORECASE)
numericPrefix = re.compile(r"^([0-9]+).*")
colorMatcher = re.compile(r"^0\s+// LEGOID\s+(\d+) - ([a-zA-Z ]+)\s*\n0 !COLOUR\s+([a-zA-Z_]+)\s+CODE\s+(\d+)",re.MULTILINE | re.IGNORECASE)

def extractBestMatch(jsonSrc, keyWords, nameNum):
	data = json.loads(jsonSrc)
	try:
		data = data['result']['typeList'][0]['items']
		results = [(result['strItemNo'],txtToKeyWords(result['strItemName'].replace("\n","")))
			for result in data if result['typeItem'].lower() == 'p']

		
		bestResult = 0
		bestName = None
		for foundName, foundKeyWords in results:
			foundNum =int(numericPrefix.match(foundName).group(1))
			overlap = foundKeyWords & keyWords
			thisResult = len(overlap)
			print(foundName, foundNum, thisResult, overlap)
			if foundNum == nameNum:
				if thisResult > bestResult:
					bestResult = thisResult
					bestName = foundName
			else:
				print("\tSkipping ineligible")
		return bestName
	except LookupError:
		print(data)
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

bl_direct_url = "https://www.bricklink.com/v2/catalog/catalogitem.page?P=%s#T=C"
bl_search_url = "https://www.bricklink.com/ajax/clone/search/searchproduct.ajax?%s"

wait_for = 0.5
def fetchPartPage(partName):
	global wait_for
	partURL = bl_direct_url % partName
	with urlopen(Request(partURL,headers=headers)) as bl_result:
			status = bl_result.getcode()
			print("HTTP Status %d" % status)
			if bl_result.geturl() == partURL:
				if status == 200:
					info = bl_result.info()
					charset = encoding.search(str(info)).group(1)
					return bl_result.read().decode(charset)
				else:
					wait_for *= 2
					print("Backing off %f" % wait_for)
					time.sleep(wait_for)
			else:
				print("Was redirected")
				if bl_result.getcode() >= 400:
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
			print("HTTP Status %d" % status)
			if bl_result.geturl() == searchURL:
				if status == 200:
					info = bl_result.info()
					charset = json_encoding.search(str(info)).group(1)
					return bl_result.read().decode(charset)
				else:
					wait_for *= 2
					print("Backing off %f" % wait_for)
					time.sleep(wait_for)
			else:
				print("Was redirected")
				if bl_result.getcode() >= 400:
					wait_for *= 2
					print("Backing off %f" % wait_for)
					time.sleep(wait_for)
	return None
					
ldraw2bricklink = {"2-sided" : "Dual Sided", "minifig" : "Minifigure"}

def cleanLine(line):
	line = line.replace("(","")
	line = line.replace(")","")
	line = line.replace("-","")
	line = line.replace("/","")
	line = line.replace(",","")
	return line

def txtToKeyWords(line):
	return {word for word in cleanLine(line).lower().split(" ") if word}

def main(faces, colortable, output, begin = 0):
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
			
			model=""
			with open(face,'r') as model_handle:
				model = model_handle.read()
			if model[:11] == "0 ~Moved to":
				print("\tSkipping defunct part %s" % name)
				continue
			elif "2-Sided" in model.splitlines()[0]:
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
					nameLine = nameLine.split(" ")[1:]
					nameWords = [cleanLine(ldraw2bricklink.get(phrase.lower(),phrase))
								for phrase in nameLine if phrase.lower() not in {"pattern","/"}]
					nameWords = [word for word in nameWords if word]
					nameLine = " ".join(nameWords)
					if len(nameLine) > 75:
						cutAt = nameLine.rfind(" ",0,75)
						nameLine = nameLine[:cutAt]
					
					keyWords = set(nameLine.lower().split(" "))
					maybeMoreKeyWords = allKeyWords.search(model)
					if maybeMoreKeyWords:
						keyWords |= txtToKeyWords(maybeMoreKeyWords.group(1))
						
					searchData = fetchSearchPage(nameLine)
					if searchData:
						bestName = extractBestMatch(searchData, keyWords, nameNum)
						if bestName:
							data = fetchPartPage(bestName)
							if data:
								out_line[2] = bestName
								out_line[3] = extractKnownColors(data)
			if out_line[3]:
				out_line[4] = []
				for (code, name, _) in out_line[3]:
					greyed = name.replace("Gray","Grey")
					if greyed.lower() in colortable_backup:
							out_line[4].append((colortable_backup[greyed.lower()],greyed.replace(" ","_")))
					else:
						print("No fall-back available, send help")
						out_line[6] = "*"
			if not out_line[2]:
				out_line[6] = "*"
			writer.writerow(out_line)
			time.sleep(0.5)
			if wait_for > 4:
				print("back off too long, stopping after %d",i)
				break
			
			
if __name__ == '__main__':
	import sys
	data = []
	with open(sys.argv[1],'r') as files_list:
		data = files_list.read().splitlines()
	colorTable = {}
	with open(sys.argv[2],'r') as ldconfig:
		colorData = ldconfig.read()
		colorData = colorMatcher.findall(colorData)
		colorTable = {blCode : (blName, ldCode, ldName) for (blCode, blName, ldName, ldCode) in colorData}
	outfile = sys.argv[3]
	start = sys.argv[4] if len(sys.argv) > 4 else 0
	main(data, colorTable, outfile, start)