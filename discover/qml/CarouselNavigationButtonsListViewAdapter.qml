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

    animated: {
        if (view) {
            // Zero duration is a trick used to reset currentIndex without animations
            return view.highlightMoveDuration !== 0;
        } else {
            return false;
        }
    }

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
