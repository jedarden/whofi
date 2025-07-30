#!/usr/bin/env python3
"""
Test script to verify YouTube Transcript API usage
"""

from youtube_transcript_api import YouTubeTranscriptApi

# Test with a known video that should have transcripts
test_video_id = "dQw4w9WgXcQ"  # Rick Roll - known to have auto-generated captions

try:
    print("🔍 Testing YouTube Transcript API...")
    print(f"Video ID: {test_video_id}")
    
    # Try the correct method
    transcript = YouTubeTranscriptApi.get_transcript(test_video_id)
    print(f"✅ Found transcript with {len(transcript)} entries")
    
    # Show first few entries
    for i, entry in enumerate(transcript[:3]):
        print(f"  {i+1}. [{entry['start']:.1f}s] {entry['text']}")
        
except Exception as e:
    print(f"❌ Error: {e}")
    
# Also test list_transcripts if it exists
try:
    print("\n🔍 Testing list_transcripts...")
    transcript_list = YouTubeTranscriptApi.list_transcripts(test_video_id)
    print(f"✅ Found transcript list: {transcript_list}")
except Exception as e:
    print(f"❌ list_transcripts error: {e}")

# Test with a more recent educational video
educational_video = "LxFkOdQGade"  # Example educational content
try:
    print(f"\n🔍 Testing with educational video: {educational_video}")
    transcript = YouTubeTranscriptApi.get_transcript(educational_video)
    print(f"✅ Found transcript with {len(transcript)} entries")
except Exception as e:
    print(f"❌ Error: {e}")