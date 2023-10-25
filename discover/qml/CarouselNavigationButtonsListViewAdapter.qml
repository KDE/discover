/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick

/*
 * Helper component to integrate CarouselNavigationButtons with a ListView.
 */
CarouselNavigationButtons {
    id: root

    required property ListView view

    // We experimented with ListView::atXBeginning/atXEnd, but ultimately
    // binding on currentIndex provides smoother experience overall.

    atBeginning: {
        if (view) {
            return view.currentIndex === 0;
        } else {
            return false;
        }
    }

    atEnd: {
        if (view) {
            return view.currentIndex === view.count - 1;
        } else {
            return false;
        }
    }

    onDecrementCurrentIndex: {
        view.decrementCurrentIndex();
    }
    onIncrementCurrentIndex: {
        view.incrementCurrentIndex();
    }
}
