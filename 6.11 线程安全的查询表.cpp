#include <list>
#include <memory>
#include <utility>
#include <shared_mutex>
#include <algorithm>
#include <vector>
#include <map>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class threadsafe_lookup_table
{
public:
protected:
private:
	class bucket_type
	{
	public:
		Value value_for(const Key &key, const Value &default_value)
		{
			std::shared_lock<std::shared_mutex> sk(m_smutex);
			bucket_iterator const found_entry = find_entry_for(key);
			return (found_entry == m_data.end() ? default_value : found_entry->second);
		}

		Value value_for(const Key &key, const Value &default_value)const
		{
			std::shared_lock<std::shared_mutex> sk(m_smutex);
			typename bucket_data::const_iterator found_entry = find_entry_for(key);
			return (found_entry == m_data.end() ? default_value : found_entry->second);
		}

		void add_or_update_mapping(const Key &key, const Value &value)
		{
			std::unique_lock<std::shared_mutex> uk(m_smutex);
			bucket_iterator const found_entry = find_entry_for(key);
			if (found_entry == m_data.end())
			{
				m_data.push_back(bucket_value(key, value));
			}
			else
			{
				found_entry->second = value;
			}
		}

		void remove_mapping(const Key &key)
		{
			std::unique_lock<std::shared_mutex> uk(m_smutex);
			bucket_iterator found_entry = find_entry_for(key);
			if (found_entry != m_data.end())
			{
				m_data.erase(found_entry);
			}
		}
		using bucket_value = std::pair<Key, Value>;
		using bucket_data = std::list<bucket_value>;
		using bucket_iterator = typename bucket_data::iterator;

		bucket_data m_data;
		mutable std::shared_mutex m_smutex;
	private:
		
		bucket_iterator find_entry_for(const Key& key)
		{
			return std::find_if(m_data.begin(), m_data.end(), [&](const bucket_value &item) { return item.first == key; });
		};

		typename bucket_data::const_iterator find_entry_for(const Key& key)const
		{
			return std::find_if(m_data.begin(), m_data.end(), [&](const bucket_value &item) { return item.first == key; });
		}
	};

	std::vector<std::unique_ptr<bucket_type>> buckets;
	Hash hasher;
	bucket_type &get_bucket(const Key &key)const
	{
		std::size_t const bucket_index = hasher(key) % buckets.size();
		return *buckets[bucket_index];
	}

public:
	using key_type = Key;
	using mapped_type = Value;
	using hash_type = Hash;

	threadsafe_lookup_table(unsigned num_buckets = 19, const Hash &hasher_ = Hash())
		: buckets(num_buckets), hasher(hasher_)
	{
		for (size_t index = 0; index < num_buckets; ++index)
		{
			buckets[index].reset(new bucket_type());
		}
	}

	threadsafe_lookup_table(const threadsafe_lookup_table&) = delete;
	threadsafe_lookup_table &operator=(const threadsafe_lookup_table&) = delete;

	Value value_for(const Key &key, const Value &default_value = Value())const
	{
		return get_bucket(key).value_for(key, default_value);
	}

	void add_for_update_mapping(const Key &key, const Value &value)
	{
		get_bucket(key).add_or_update_mapping(key, value);
	}

	void remove_mapping(const Key &key)
	{
		get_bucket(key).remove_mapping(key);
	}

	std::map<Key, Value> get_map()const
	{
		std::vector<std::unique_lock<std::shared_mutex>> vecUKs;
		for (size_t index = 0; index < buckets.size(); ++index)
		{
			vecUKs.push_back(std::unique_lock<std::shared_mutex>(buckets[index]->m_smutex));
		}

		std::map<Key, Value> res;
		for (size_t index = 0; index < buckets.size(); ++index)
		{
			for (auto it = buckets[index]->m_data.cbegin(); it != buckets[index]->m_data.cend(); ++it)
			{
				res.insert(*it);
			}
		}

		return res;
	}
};

int main()
{
	threadsafe_lookup_table<int, int> tlt;
	tlt.add_for_update_mapping(1, 2);
	tlt.add_for_update_mapping(3, 2);
	tlt.value_for(3, 3);
	tlt.remove_mapping(3);

	tlt.get_map();

	const threadsafe_lookup_table<int, int> &tl = tlt;
	tl.value_for(1, 2);
	tl.get_map();

	return EXIT_SUCCESS;
}