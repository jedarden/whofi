#!/usr/bin/env python3
"""
YouTube Research Script for whofi Project
Searches for whofi-related videos and downloads transcripts
"""

import json
import os
import re
import requests
from urllib.parse import urlparse, parse_qs
from youtube_transcript_api import YouTubeTranscriptApi
from datetime import datetime

class YouTubeWhofiResearcher:
    def __init__(self, output_dir="/workspaces/scratchpad/whofi/transcripts"):
        self.output_dir = output_dir
        os.makedirs(output_dir, exist_ok=True)
        
        # Search terms for whofi-related content
        self.search_terms = [
            "whofi",
            "WiFi fingerprinting",
            "ESP32 CSI",
            "WiFi CSI fingerprinting",
            "802.11 CSI",
            "Channel State Information WiFi",
            "WiFi positioning system",
            "ESP32 WiFi localization",
            "WiFi based indoor positioning",
            "CSI based device fingerprinting"
        ]
        
        self.found_videos = []
        self.processed_videos = []
    
    def extract_video_id(self, url):
        """Extract YouTube video ID from various URL formats"""
        patterns = [
            r'(?:v=|\/)([0-9A-Za-z_-]{11}).*',
            r'(?:embed\/)([0-9A-Za-z_-]{11})',
            r'(?:youtu\.be\/)([0-9A-Za-z_-]{11})'
        ]
        
        for pattern in patterns:
            match = re.search(pattern, url)
            if match:
                return match.group(1)
        return None
    
    def search_youtube_manual(self):
        """Manual list of whofi-related YouTube videos (since we can't use YouTube API)"""
        # These are known videos that would come up in searches
        known_videos = [
            {
                "video_id": "dQw4w9WgXcQ",  # Placeholder - would be replaced with actual whofi videos
                "title": "Example: WiFi CSI Fingerprinting Demo",
                "url": "https://www.youtube.com/watch?v=dQw4w9WgXcQ",
                "description": "Demonstration of WiFi fingerprinting using CSI data"
            }
        ]
        
        print("🔍 Searching for whofi-related YouTube videos...")
        print("📝 Note: In a real implementation, this would use YouTube Data API")
        print("🎯 Search terms:", ", ".join(self.search_terms))
        
        # For demo purposes, we'll simulate finding videos
        print("\n📹 Videos that would be found with these search terms:")
        for term in self.search_terms:
            print(f"   - Searching: '{term}'")
        
        return known_videos
    
    def get_video_transcript(self, video_id):
        """Download transcript for a YouTube video"""
        try:
            print(f"📥 Downloading transcript for video ID: {video_id}")
            
            # Try to get transcript
            transcript_list = YouTubeTranscriptApi.get_transcript(video_id)
            
            # Combine transcript parts
            full_transcript = ""
            for entry in transcript_list:
                timestamp = entry['start']
                text = entry['text']
                full_transcript += f"[{timestamp:.1f}s] {text}\n"
            
            return full_transcript
            
        except Exception as e:
            print(f"❌ Error downloading transcript for {video_id}: {str(e)}")
            return None
    
    def save_transcript(self, video_info, transcript):
        """Save transcript to file with metadata"""
        video_id = video_info['video_id']
        title = re.sub(r'[<>:"/\\|?*]', '_', video_info['title'])
        filename = f"{video_id}_{title}.txt"
        filepath = os.path.join(self.output_dir, filename)
        
        # Create metadata header
        metadata = f"""# YouTube Video Transcript
# Video ID: {video_id}
# Title: {video_info['title']}
# URL: {video_info['url']}
# Downloaded: {datetime.now().isoformat()}
# Description: {video_info.get('description', 'N/A')}

===== TRANSCRIPT =====

{transcript}
"""
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(metadata)
        
        print(f"💾 Saved transcript: {filename}")
        return filepath
    
    def analyze_transcript(self, transcript):
        """Extract key technical insights from transcript"""
        if not transcript:
            return {}
        
        # Keywords to look for in whofi context
        technical_keywords = {
            'csi': ['CSI', 'Channel State Information', 'channel state'],
            'fingerprinting': ['fingerprint', 'fingerprinting', 'signature'],
            'esp32': ['ESP32', 'esp32', 'ESP-32'],
            'wifi': ['wifi', 'WiFi', 'Wi-Fi', '802.11'],
            'positioning': ['position', 'location', 'localization', 'tracking'],
            'machine_learning': ['machine learning', 'ML', 'neural network', 'classification'],
            'accuracy': ['accuracy', 'precision', 'performance', 'error rate'],
            'indoor': ['indoor', 'indoor positioning', 'indoor navigation']
        }
        
        insights = {}
        for category, keywords in technical_keywords.items():
            count = 0
            mentions = []
            for keyword in keywords:
                matches = re.findall(rf'\b{re.escape(keyword)}\b', transcript, re.IGNORECASE)
                count += len(matches)
                if matches:
                    mentions.extend([keyword] * len(matches))
            
            insights[category] = {
                'count': count,
                'mentions': list(set(mentions))
            }
        
        return insights
    
    def generate_summary(self):
        """Generate summary of research findings"""
        summary = {
            'research_date': datetime.now().isoformat(),
            'search_terms_used': self.search_terms,
            'videos_found': len(self.found_videos),
            'transcripts_downloaded': len(self.processed_videos),
            'output_directory': self.output_dir,
            'videos_processed': self.processed_videos
        }
        
        summary_path = os.path.join(self.output_dir, 'research_summary.json')
        with open(summary_path, 'w', encoding='utf-8') as f:
            json.dump(summary, f, indent=2)
        
        return summary
    
    def run_research(self):
        """Main research workflow"""
        print("🚀 Starting YouTube whofi research...")
        
        # Step 1: Search for videos
        videos = self.search_youtube_manual()
        self.found_videos = videos
        
        # Step 2: Process each video
        for video_info in videos:
            video_id = video_info['video_id']
            print(f"\n🎬 Processing: {video_info['title']}")
            
            # Download transcript
            transcript = self.get_video_transcript(video_id)
            
            if transcript:
                # Save transcript
                filepath = self.save_transcript(video_info, transcript)
                
                # Analyze content
                insights = self.analyze_transcript(transcript)
                
                # Store processing info
                self.processed_videos.append({
                    'video_info': video_info,
                    'transcript_file': filepath,
                    'insights': insights,
                    'transcript_length': len(transcript.split()),
                    'processed_date': datetime.now().isoformat()
                })
            
            else:
                print(f"⚠️  Could not process transcript for {video_id}")
        
        # Step 3: Generate summary
        summary = self.generate_summary()
        
        print(f"\n✅ Research complete!")
        print(f"📊 Summary: {summary['transcripts_downloaded']} transcripts downloaded")
        print(f"📁 Files saved to: {self.output_dir}")
        
        return summary

if __name__ == "__main__":
    researcher = YouTubeWhofiResearcher()
    summary = researcher.run_research()
    print("\n📋 Research Summary:")
    print(json.dumps(summary, indent=2))