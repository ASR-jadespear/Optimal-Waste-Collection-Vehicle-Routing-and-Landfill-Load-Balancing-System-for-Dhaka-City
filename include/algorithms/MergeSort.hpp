#pragma once

#include "core/CollectionPoint.hpp"
#include <vector>
#include <functional>

namespace dhaka
{

    // Pure custom MergeSort implementation (Fundamental Algorithm)
    class MergeSort
    {
    public:
        // Sorts a vector of CollectionPoint pointers by urgency score (descending by default)
        static void sortByUrgency(std::vector<CollectionPoint *> &points, bool descending = true);

        // Generic MergeSort with custom comparator
        template <typename T, typename Compare>
        static void sort(std::vector<T> &arr, Compare comp)
        {
            if (arr.size() <= 1)
                return;
            std::vector<T> temp(arr.size());
            mergeSortInternal(arr, temp, 0, static_cast<int>(arr.size()) - 1, comp);
        }

    private:
        static void mergePoints(std::vector<CollectionPoint *> &arr,
                                std::vector<CollectionPoint *> &temp,
                                int left, int mid, int right, bool descending);

        static void mergeSortPointsInternal(std::vector<CollectionPoint *> &arr,
                                            std::vector<CollectionPoint *> &temp,
                                            int left, int right, bool descending);

        template <typename T, typename Compare>
        static void mergeInternal(std::vector<T> &arr, std::vector<T> &temp,
                                  int left, int mid, int right, Compare comp)
        {
            int i = left;
            int j = mid + 1;
            int k = left;

            while (i <= mid && j <= right)
            {
                if (comp(arr[i], arr[j]))
                {
                    temp[k++] = arr[i++];
                }
                else
                {
                    temp[k++] = arr[j++];
                }
            }

            while (i <= mid)
                temp[k++] = arr[i++];
            while (j <= right)
                temp[k++] = arr[j++];
            for (int p = left; p <= right; ++p)
                arr[p] = temp[p];
        }

        template <typename T, typename Compare>
        static void mergeSortInternal(std::vector<T> &arr, std::vector<T> &temp,
                                      int left, int right, Compare comp)
        {
            if (left >= right)
                return;
            int mid = left + (right - left) / 2;
            mergeSortInternal(arr, temp, left, mid, comp);
            mergeSortInternal(arr, temp, mid + 1, right, comp);
            mergeInternal(arr, temp, left, mid, right, comp);
        }
    };

} // namespace dhaka
