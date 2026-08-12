"""OpenAI-compatible example with CachyLlama's request-scoped cache/user fields."""
from __future__ import annotations

import os
from openai import OpenAI

client = OpenAI(
    base_url=os.environ.get("BASE_URL", "http://127.0.0.1:9090/v1"),
    api_key=os.environ.get("API_KEY", "change-me"),
)

response = client.chat.completions.create(
    model="configured-model-name",
    messages=[
        {"role": "system", "content": "Answer with compact, auditable reasoning."},
        {"role": "user", "content": "Summarize the cache design."},
    ],
    extra_body={
        "cache_prompt": True,
        "llama_user_id": "tenant42-user7",
    },
)
print(response.choices[0].message.content)
