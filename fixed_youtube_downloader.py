#!/usr/bin/env python3
"""
Fixed YouTube Transcript Downloader for whofi Research
Uses correct YouTube Transcript API instantiation
"""

import json
import os
import re
from datetime import datetime
from youtube_transcript_api import YouTubeTranscriptApi
from whofi_videos_list import get_whofi_videos, get_search_terms

class YouTubeTranscriptDownloader:
    def __init__(self, output_dir="/workspaces/scratchpad/whofi/transcripts"):
        self.output_dir = output_dir
        os.makedirs(output_dir, exist_ok=True)
        
        # Initialize the API
        self.api = YouTubeTranscriptApi()
        
        self.results = {
            'successful_downloads': [],
            'failed_downloads': [],
            'analysis_results': {},
            'research_metadata': {
                'timestamp': datetime.now().isoformat(),
                'search_terms': get_search_terms(),
                'total_videos_attempted': 0,
                'successful_transcripts': 0
            }
        }
    
    def download_transcript(self, video_id, video_info):
        """Download transcript for a specific video"""
        try:
            print(f"📥 Downloading transcript for: {video_info['title']}")
            print(f"   Video ID: {video_id}")
            
            # Method 1: Try using the simple fetch method
            try:
                transcript_data = self.api.fetch(video_id, languages=['en'])
                print("✅ Found English transcript")
            except:
                # Method 2: Try getting any available transcript
                try:
                    transcript_list = self.api.list(video_id)
                    # Get the first available transcript
                    for transcript in transcript_list:
                        transcript_data = transcript.fetch()
                        print(f"✅ Found transcript in {transcript.language}")
                        break
                except Exception as e:
                    print(f"❌ No transcripts available: {str(e)}")
                    raise e
            
            if transcript_data:
                # Format transcript
                formatted_transcript = self.format_transcript(transcript_data)
                
                # Save to file
                filename = self.save_transcript_file(video_id, video_info, formatted_transcript)
                
                # Analyze content
                analysis = self.analyze_transcript_content(formatted_transcript)
                
                success_info = {
                    'video_id': video_id,
                    'video_info': video_info,
                    'transcript_file': filename,
                    'word_count': len(formatted_transcript.split()),
                    'analysis': analysis,
                    'download_time': datetime.now().isoformat()
                }
                
                self.results['successful_downloads'].append(success_info)
                print(f"✅ Successfully downloaded: {filename}")
                return True
            
        except Exception as e:
            error_info = {
                'video_id': video_id,
                'video_info': video_info,
                'error': str(e),
                'error_time': datetime.now().isoformat()
            }
            self.results['failed_downloads'].append(error_info)
            print(f"❌ Failed to download transcript for {video_id}: {str(e)}")
            return False
    
    def format_transcript(self, transcript_data):
        """Format transcript data into readable text"""
        formatted_text = ""
        
        # Handle both list of dicts and FetchedTranscript object
        if hasattr(transcript_data, '__iter__'):
            entries = transcript_data
        else:
            entries = transcript_data.raw
        
        for entry in entries:
            start_time = entry.get('start', 0)
            text = entry.get('text', '')
            # Format timestamp as minutes:seconds
            minutes = int(start_time // 60)
            seconds = int(start_time % 60)
            formatted_text += f"[{minutes:02d}:{seconds:02d}] {text}\n"
        
        return formatted_text
    
    def save_transcript_file(self, video_id, video_info, transcript_content):
        """Save transcript to file with metadata"""
        # Create safe filename
        title = re.sub(r'[<>:"/\\|?*]', '_', video_info['title'])[:50]
        filename = f"{video_id}_{title}.txt"
        filepath = os.path.join(self.output_dir, filename)
        
        # Create file content with metadata
        file_content = f"""# YouTube Video Transcript - whofi Research
# 
# Video ID: {video_id}
# Title: {video_info['title']}
# URL: {video_info['url']}
# Category: {video_info.get('category', 'unknown')}
# Relevance: {video_info.get('relevance', 'unknown')}
# Description: {video_info.get('description', 'N/A')}
# Downloaded: {datetime.now().isoformat()}
#
# ============================================================

{transcript_content}
"""
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(file_content)
        
        return filename
    
    def analyze_transcript_content(self, transcript):
        """Analyze transcript content for whofi-related keywords and concepts"""
        if not transcript:
            return {}
        
        # Define keyword categories relevant to whofi
        keyword_categories = {
            'whofi_specific': [
                'whofi', 'who-fi', 'device fingerprinting', 'fingerprint'
            ],
            'wifi_tech': [
                'wifi', 'wi-fi', '802.11', 'wireless', 'wlan', 'access point', 'router'
            ],
            'csi_concepts': [
                'csi', 'channel state information', 'channel state', 'channel estimation',
                'frequency response', 'amplitude', 'phase'
            ],
            'esp32_hardware': [
                'esp32', 'esp-32', 'espressif', 'microcontroller', 'iot device'
            ],
            'positioning': [
                'position', 'positioning', 'location', 'localization', 'indoor positioning',
                'navigation', 'tracking', 'coordinates'
            ],
            'machine_learning': [
                'machine learning', 'ml', 'neural network', 'classification', 'training',
                'model', 'algorithm', 'prediction', 'feature extraction'
            ],
            'security': [
                'security', 'authentication', 'identification', 'privacy', 'detection',
                'surveillance', 'monitoring'
            ],
            'technical_metrics': [
                'accuracy', 'precision', 'performance', 'error rate', 'reliability',
                'throughput', 'latency', 'range', 'signal strength', 'rssi'
            ]
        }
        
        analysis = {}
        transcript_lower = transcript.lower()
        
        for category, keywords in keyword_categories.items():
            matches = []
            total_count = 0
            
            for keyword in keywords:
                count = len(re.findall(r'\b' + re.escape(keyword.lower()) + r'\b', transcript_lower))
                if count > 0:
                    matches.append({'keyword': keyword, 'count': count})
                    total_count += count
            
            analysis[category] = {
                'total_mentions': total_count,
                'keyword_matches': matches,
                'relevance_percentage': round((total_count / len(transcript.split())) * 100, 2) if transcript.split() else 0
            }
        
        # Calculate overall relevance score
        relevance_score = sum(cat['total_mentions'] for cat in analysis.values())
        analysis['overall_relevance_score'] = relevance_score
        
        return analysis
    
    def generate_summary_report(self):
        """Generate comprehensive summary report"""
        report_path = os.path.join(self.output_dir, 'whofi_youtube_research_report.json')
        
        # Add analysis summary
        if self.results['successful_downloads']:
            all_analyses = [item['analysis'] for item in self.results['successful_downloads']]
            
            # Aggregate keyword analysis
            aggregated_analysis = {}
            total_videos = len(all_analyses)
            
            for analysis in all_analyses:
                for category, data in analysis.items():
                    if category == 'overall_relevance_score':
                        continue
                    
                    if category not in aggregated_analysis:
                        aggregated_analysis[category] = {
                            'total_mentions': 0, 
                            'videos_mentioning': 0,
                            'average_mentions_per_video': 0,
                            'top_keywords': {}
                        }
                    
                    if data['total_mentions'] > 0:
                        aggregated_analysis[category]['total_mentions'] += data['total_mentions']
                        aggregated_analysis[category]['videos_mentioning'] += 1
                        
                        # Track top keywords
                        for match in data['keyword_matches']:
                            keyword = match['keyword']
                            count = match['count']
                            if keyword not in aggregated_analysis[category]['top_keywords']:
                                aggregated_analysis[category]['top_keywords'][keyword] = 0
                            aggregated_analysis[category]['top_keywords'][keyword] += count
            
            # Calculate averages
            for category in aggregated_analysis:
                total_mentions = aggregated_analysis[category]['total_mentions']
                aggregated_analysis[category]['average_mentions_per_video'] = round(total_mentions / total_videos, 2)
            
            self.results['aggregated_analysis'] = aggregated_analysis
        
        with open(report_path, 'w', encoding='utf-8') as f:
            json.dump(self.results, f, indent=2, ensure_ascii=False)
        
        print(f"📊 Research report saved: whofi_youtube_research_report.json")
        return report_path
    
    def run_download_session(self):
        """Main method to download all transcripts"""
        print("🚀 Starting YouTube transcript download session for whofi research")
        print("=" * 60)
        
        videos = get_whofi_videos()
        self.results['research_metadata']['total_videos_attempted'] = len(videos)
        
        print(f"📋 Found {len(videos)} videos to process")
        print()
        
        for i, video_info in enumerate(videos, 1):
            print(f"🎬 Processing video {i}/{len(videos)}")
            video_id = video_info['video_id']
            
            success = self.download_transcript(video_id, video_info)
            if success:
                self.results['research_metadata']['successful_transcripts'] += 1
            
            print("-" * 40)
        
        # Generate summary report
        report_path = self.generate_summary_report()
        
        print(f"\n✅ Download session complete!")
        print(f"📊 Results: {self.results['research_metadata']['successful_transcripts']}/{len(videos)} transcripts downloaded")
        print(f"📁 Files saved to: {self.output_dir}")
        print(f"📋 Report saved to: {report_path}")
        
        return self.results

# Also create a simple test with a known working video
def test_api_with_known_video():
    """Test with a video known to have transcripts"""
    print("🧪 Testing API with known working video...")
    
    # Use a popular educational video that likely has transcripts
    test_videos = [
        "dQw4w9WgXcQ",  # Rick Roll (auto-generated captions)
        "9bZkp7q19f0",  # Popular PSY - Gangnam Style (has multiple language subs)
    ]
    
    api = YouTubeTranscriptApi()
    
    for video_id in test_videos:
        try:
            print(f"\n🔍 Testing video: {video_id}")
            transcript_list = api.list(video_id)
            print(f"✅ Found transcripts for {video_id}")
            
            # Try to get English transcript
            for transcript in transcript_list:
                print(f"   Available: {transcript.language} ({transcript.language_code})")
                if transcript.language_code == 'en':
                    data = transcript.fetch()
                    print(f"   ✅ Downloaded English transcript: {len(data)} entries")
                    # Show first entry
                    if data:
                        print(f"   Sample: [{data[0]['start']:.1f}s] {data[0]['text'][:50]}...")
                    break
            
        except Exception as e:
            print(f"   ❌ Error with {video_id}: {str(e)}")

if __name__ == "__main__":
    # First test the API
    test_api_with_known_video()
    
    print("\n" + "=" * 60)
    
    # Then run the main research
    downloader = YouTubeTranscriptDownloader()
    results = downloader.run_download_session()
    
    # Print summary
    print("\n" + "=" * 60)
    print("📈 RESEARCH SUMMARY")
    print("=" * 60)
    
    if results['successful_downloads']:
        print("✅ Successfully downloaded transcripts:")
        for item in results['successful_downloads']:
            print(f"   • {item['video_info']['title']}")
            print(f"     Words: {item['word_count']}, Relevance: {item['analysis']['overall_relevance_score']}")
    
    if results['failed_downloads']:
        print("\n❌ Failed downloads:")
        for item in results['failed_downloads']:
            print(f"   • {item['video_info']['title']}: {item['error']}")